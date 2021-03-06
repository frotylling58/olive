/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "sequence.h"

#include <QCoreApplication>

#include "debug.h"

Sequence::Sequence() {
  playhead = 0;
  using_workarea = false;
  workarea_in = 0;
  workarea_out = 0;
  wrapper_sequence = false;
}

Sequence::~Sequence() {}

SequencePtr Sequence::copy() {
  SequencePtr s(new Sequence());
  s->name = QCoreApplication::translate("Sequence", "%1 (copy)").arg(name);
  s->width = width;
  s->height = height;
  s->frame_rate = frame_rate;
  s->audio_frequency = audio_frequency;
  s->audio_layout = audio_layout;

  // deep copy all of the sequence's clips
  s->clips.resize(clips.size());
  for (int i=0;i<clips.size();i++) {
    ClipPtr c = clips.at(i);
    if (c == nullptr) {
      s->clips[i] = nullptr;
    } else {
      ClipPtr copy = c->copy(s);
      copy->linked = c->linked;
      s->clips[i] = copy;
    }
  }

  // copy all of the sequence's markers
  s->markers = markers;

  return s;
}

long Sequence::getEndFrame() {
  long end = 0;
  for (int j=0;j<clips.size();j++) {
    ClipPtr c = clips.at(j);
    if (c != nullptr && c->timeline_out() > end) {
      end = c->timeline_out();
    }
  }
  return end;
}

void Sequence::RefreshClips(Media *m) {
  for (int i=0;i<clips.size();i++) {
    ClipPtr c = clips.at(i);

    if (c != nullptr
        && (m == nullptr || c->media() == m)) {
      c->Close(true);
      c->refresh();
    }
  }
}

QVector<Clip *> Sequence::SelectedClips()
{
  QVector<Clip*> selected_clips;

  for (int i=0;i<clips.size();i++) {
    Clip* c = clips.at(i).get();
    if (c != nullptr && IsClipSelected(c, true)) {
      selected_clips.append(c);
    }
  }

  return selected_clips;
}

QVector<int> Sequence::SelectedClipIndexes()
{
  QVector<int> selected_clips;

  for (int i=0;i<clips.size();i++) {
    Clip* c = clips.at(i).get();
    if (c != nullptr && IsClipSelected(c, true)) {
      selected_clips.append(i);
    }
  }

  return selected_clips;
}

bool Sequence::IsClipSelected(int clip_index, bool containing)
{
  return IsClipSelected(clips.at(clip_index).get(), containing);
}

bool Sequence::IsClipSelected(Clip *clip, bool containing)
{
  for (int i=0;i<selections.size();i++) {
    const Selection& s = selections.at(i);
    if (clip->track() == s.track && ((clip->timeline_in() >= s.in && clip->timeline_out() <= s.out && containing)
                                  || (!containing && !(clip->timeline_in() < s.in && clip->timeline_out() < s.in)
                                   && !(clip->timeline_in() > s.in && clip->timeline_out() > s.in)))) {
      return true;
    }
  }
  return false;
}

void Sequence::getTrackLimits(int* video_tracks, int* audio_tracks) {
  int vt = 0;
  int at = 0;
  for (int j=0;j<clips.size();j++) {
    ClipPtr c = clips.at(j);
    if (c != nullptr) {
      if (c->track() < 0 && c->track() < vt) { // video clip
        vt = c->track();
      } else if (c->track() > at) {
        at = c->track();
      }
    }
  }
  if (video_tracks != nullptr) *video_tracks = vt;
  if (audio_tracks != nullptr) *audio_tracks = at;
}

// static variable for the currently active sequence
SequencePtr olive::ActiveSequence = nullptr;
