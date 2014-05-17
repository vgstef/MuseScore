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

#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/system.h"
#include "libmscore/staff.h"
#include "libmscore/page.h"
#include "libmscore/sym.h"
#include "libmscore/instrument.h"
#include "libmscore/part.h"
#include "libmscore/timesig.h"
#include "libmscore/keysig.h"
#include "libmscore/barline.h"

#include "scoreview.h"
#include "continuouspanel.h"

namespace Ms {

//---------------------------------------------------------
//   paintContinousPanel
//---------------------------------------------------------

void ContinuousPanel::paint(const QRect& r, QPainter& p)
      {
      if (!_visible)
            return;
      Measure* measure = _score->tick2measure(0);
      if (measure == 0)
            return;
      Segment* s = measure->first(Segment::SegChordRest);
      double _spatium = _score->spatium();
      x = 0;
      System* system = measure->system();
      if (system == 0)
            return;


      if (_width <= 0)
            _width  = s->x();
            //_width  = s->canvasPos().x();

      //
      // Don't show panel if staff names are visible
      //
      if (_sv->xoffset() / _sv->mag() + _width >= 0)
           return;

      //
      // Set panel height for whole system
      //
      _height  = 6 * _spatium;
      y = system->staffYpage(0) + system->page()->pos().y();
      double y2 = 0.0;
      for (int i = 0; i < _score->nstaves(); ++i) {
            SysStaff* ss = system->staff(i);
            if (!ss->show() || !_score->staff(i)->show())
                  continue;
            y2 = ss->y() + ss->bbox().height();
            }
      _height += y2 + 2*_spatium;
      y -= 4 * _spatium;

      //
      // Check elements at current panel position
      //
      _offsetPanel = -(_sv->xoffset()) / _sv->mag();
      _rect = QRect(_offsetPanel + _width-1, y, 1, _height);
      //qDebug() << "width=" << _width << "x="<< x << "y="<< y << "_offsetPanel=" << _offsetPanel << "_sv->xoffset()" << _sv->xoffset() << "_sv->mag()" << _sv->mag() <<"_spatium" << _spatium << "s->canvasPos().x()" << s->canvasPos().x() << "s->x()" << s->x();
      Page* page = _score->pages().front();
      QList<Element*> elementsCurrent = page->items(_rect);
      qStableSort(elementsCurrent.begin(), elementsCurrent.end(), elementLessThan);

      foreach(const Element* e, elementsCurrent) {
            e->itemDiscovered = 0;
            if (!e->visible()) {
                  if (_score->printing() || !_score->showInvisible())
                        continue;
                  }

            if (e->type() == Element::MEASURE) {
                  _currentMeasure = static_cast<const Measure*>(e);
                  _currentTimeSig = _currentMeasure->timesig();
                  _currentMeasureTick = _currentMeasure->tick();
                  _currentMeasureNo = _currentMeasure->no();
                  //qDebug() << "ElementCurrent type #"<<e->type() << "  "<<e->name(e->type()) << "  Measure #"<<_currentMeasure->no() << "  TimeSig "<<_currentMeasure->timesig().print();
                  break;
                  }          
            }
      findElementWidths(elementsCurrent);
      draw(p, elementsCurrent);
      }


//---------------------------------------------------------
//   findElementWidths
//      determines the max width for each element types
//---------------------------------------------------------

void ContinuousPanel::findElementWidths(const QList<Element*>& el) {
      //
      // The first pass serves to get the maximum width for each elements
      //
      heightName = 0;
      widthClef = 0;
      widthKeySig = 0;
      widthTimeSig = 0;
      foreach(const Element* e, el) {
            e->itemDiscovered = 0;
            if (!e->visible()) {
                  if (_score->printing() || !_score->showInvisible())
                        continue;
                  }
            //qDebug() << "ElementFirst =  type #"<<e->type() << "  "<<e->name(e->type());

           if (e->type() == Element::STAFF_LINES) {
                  Staff* currentStaff = _score->staff(e->staffIdx());

                  //
                  // Find maximum height for the staff name
                  //
                  QList<StaffName>& staffNamesShort = currentStaff->part()->instr()->shortNames();
                  QString staffNameShort = staffNamesShort.isEmpty() ? "" : staffNamesShort[0].name;
                  Text* newName = new Text(_score);
                  newName->setText(staffNameShort);
                  newName->setTrack(e->track());
                  newName->sameLayout();
                  if ((newName->height() > heightName) && (newName->text() != ""))
                        heightName = newName->height();

                  //
                  // Find maximum width for the current Clef
                  //
                  Clef* newClef = new Clef(_score);
                  ClefType currentClef = currentStaff->clef(_currentMeasureTick);
                  newClef->setClefType(currentClef);
                  newClef->layout();
                  if (newClef->width() > widthClef)
                        widthClef = newClef->width();

                  //
                  // Find maximum width for the current KeySignature
                  //
                  KeySig* newKs = new KeySig(_score);
                  KeySigEvent currentKeySigEvent = currentStaff->key(_currentMeasureTick);
                  newKs->setKeySigEvent(currentKeySigEvent);
                  newKs->layout();
                  if (newKs->width() > widthKeySig)
                        widthKeySig = newKs->width();

                  //
                  // Find maximum width for the current TimeSignature
                  //
                  TimeSig* newTs = new TimeSig(_score);
                  newTs->setSig(_currentTimeSig.numerator(), _currentTimeSig.denominator(), TSIG_NORMAL);
                  newTs->layout();
                  if (newTs->width() > widthTimeSig)
                        widthTimeSig = newTs->width();
                 }
            }

      qreal newWidth = heightName*1.5 + widthClef + widthKeySig + widthTimeSig;
      if (newWidth > 0)
            _width = newWidth+5;
      _rect = QRect(0, y, _width, _height);
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void ContinuousPanel::draw(QPainter& painter, const QList<Element*>& el) {     
      painter.save();

      //
      // Draw colored rectangle
      //
      painter.setClipping(false);
      QPointF pos(_offsetPanel, 0);
      painter.translate(pos);
      QColor c(MScore::selectColor[0]);
      QPen pen(c);
      pen.setWidthF(0.0);
      pen.setStyle( Qt::NoPen );
      painter.setPen(pen);
      painter.setOpacity(0.80);
      painter.setBrush(QColor(205, 210, 235, 255));
      painter.drawRect(_rect);
      painter.setClipRect(_rect);
      painter.setClipping(true);

      //
      // Draw measure text number
      //
      painter.setOpacity(1);
      painter.setBrush(QColor(0, 0, 0, 255));
      Text* newElement = new Text(_score);
      newElement->setTextStyleType(TEXT_STYLE_DEFAULT);
      newElement->setTrack(0);
      newElement->setParent(0);
      pos = QPointF (0, y+newElement->height());
      newElement->sameLayout();
      newElement->draw(&painter);
      QString text = QString("#%1").arg(_currentMeasureNo+1);
      painter.drawText(QPointF(0,y+newElement->height()), text);
      pos += QPointF (_offsetPanel, -y);
      painter.translate(-pos);

      //
      // This second pass draws the elements spaced evently using maximum elements width
      //
      foreach(const Element* e, el) {
            e->itemDiscovered = 0;
            if (!e->visible()) {
                  if (_score->printing() || !_score->showInvisible())
                        continue;
                  }

           if (e->type() == Element::STAFF_LINES) {
                  Staff* currentStaff = _score->staff(e->staffIdx());

                  //
                  // Draw staff lines
                  //
                  QPointF pos;
                  pos = QPointF (_offsetPanel,0);
                  pos += QPointF (heightName*1.5, e->pagePos().y());
                  painter.translate(pos);
                  e->draw(&painter);
                  painter.translate(-pos);

                  //
                  // Draw the current staff name
                  //
                  QList<StaffName>& staffNamesShort = currentStaff->part()->instr()->shortNames();               
                  QString staffNameShort = staffNamesShort.isEmpty() ? "" : staffNamesShort[0].name;
                  Text* newName = new Text(_score);
                  newName->setText(staffNameShort);
                  newName->setTrack(e->track());
                  newName->sameLayout();
                  pos = QPointF (_offsetPanel, e->pagePos().y());
                  painter.translate(pos);
                  pos = QPointF (heightName, e->height()/2+newName->width()/2);  // Because we rotate the canvas, height and width are swaped
                  painter.translate(pos);
                  painter.rotate(-90);
                  newName->draw(&painter);
                  painter.rotate(90);
                  painter.translate(-pos);
                  pos = QPointF (heightName*1.5, 0);
                  painter.translate(pos);

                  //
                  // Draw barline
                  //
                  BarLine* newBarLine = new BarLine(_score);
                  newBarLine->setBarLineType(NORMAL_BAR);
                  newBarLine->layout();
                  newBarLine->draw(&painter);

                  //
                  // Draw the current Clef
                  //
                  Clef* newClef = new Clef(_score);
                  ClefType currentClef = currentStaff->clef(_currentMeasureTick);
                  newClef->setClefType(currentClef);
                  newClef->layout();
                  newClef->draw(&painter);
                  pos = QPointF(widthClef,0);
                  painter.translate(pos);

                  //
                  // Draw the current KeySignature
                  //
                  KeySig* newKs = new KeySig(_score);
                  KeySigEvent currentKeySigEvent = currentStaff->key(_currentMeasureTick);
                  newKs->setKeySigEvent(currentKeySigEvent);
                  newKs->layout();
                  newKs->draw(&painter);
                  pos = QPointF(widthKeySig,0);
                  painter.translate(pos);

                  //
                  // Draw the current TimeSignature
                  //
                  TimeSig* newTs = new TimeSig(_score);
                  newTs->setSig(_currentTimeSig.numerator(), _currentTimeSig.denominator(), TSIG_NORMAL);
                  newTs->layout();
                  newTs->draw(&painter);
                  pos = QPointF(_offsetPanel+heightName*1.5+widthClef+widthKeySig,e->pagePos().y());
                  painter.translate(-pos);

                  //qDebug() << "Staff #" << e->staffIdx() << " _widtch "<<_width<< " offsetpanel "<<_offsetPanel;
                 }
            }
      painter.restore();
      }


//---------------------------------------------------------
//   updatePanelPosition
//---------------------------------------------------------

}
