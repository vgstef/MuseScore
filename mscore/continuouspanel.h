//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __CONTINUOUSPANEL_H__
#define __CONTINUOUSPANEL_H__

namespace Ms {

class ScoreView;

//---------------------------------------------------------
//   ContinuousPanel
//---------------------------------------------------------

class ContinuousPanel {
      ScoreView* _sv;
      QRectF _rect;
      bool _visible    { false };
      const Measure* _currentMeasure;
      int _currentMeasureTick  { 0 };
      int _currentMeasureNo  { 0 };
      Fraction _currentTimeSig;
      qreal _width { 0 };
      qreal _height { 0 };
      qreal _offsetPanel { 0 };
      qreal x=0;
      qreal y=0;
      qreal heightName = 0;
      qreal widthClef = 0;
      qreal widthKeySig = 0;
      qreal widthTimeSig = 0;

   protected:
      Score* _score;
      void findElementWidths(const QList<Element*>& el);
      void draw(QPainter& painter, const QList<Element*>& el);

   public:
      ContinuousPanel(ScoreView* sv) : _sv(sv)  { _currentMeasure = 0; }

      QRectF rect() const            { return _rect;     }
      void setRect(const QRectF& r)  { _rect = r;        }
      bool visible() const           { return _visible;  }
      void setVisible(bool val)      { _visible = val;   }
      void setScore(Score* s)        { _score = s;       }

      void paint(const QRect& r, QPainter& p);
      };

}

#endif

