/*
  Q Light Controller
  sceneitems.h

  Copyright (C) Heikki Junnila

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  Version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details. The license is
  in the file "COPYING".

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifndef SCENEITEMS_H
#define SCENEITEMS_H

#include <QGraphicsItem>
#include <QFont>
#include <QObject>

#include "chaser.h"
#include "track.h"

/*********************************************************************
 * Scene Header class. Clickable time line header
 *********************************************************************/
class SceneHeaderItem :  public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    SceneHeaderItem(int);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void setTimeScale(int val);
    int getTimeScale();
    int getTimeStep();

signals:
    void itemClicked(QGraphicsSceneMouseEvent *);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    int m_width;
    int m_timeStep;
    int m_timeScale;
};

/***************************************************************************
 * Scene Cursor class. Cursor which marks the time position in a scene
 ***************************************************************************/
class SceneCursorItem : public QGraphicsItem
{
public:
    SceneCursorItem(int h);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void setTime(quint32 t);
    quint32 getTime();
private:
    int m_height;
    quint32 m_time;
};

/****************************************************************************
 * Track class. This is only a visual item to fullfill the multi track view
 ****************************************************************************/
class TrackItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    TrackItem(Track *track, int number);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /** Return pointer to the Track class associated to this item */
    Track *getTrack();

    /** Return the track number */
    int getTrackNumber();

    /** Set the track name */
    void setName(QString name);

    /** Enable/disable active state which higlight the left bar */
    void setActive(bool flag);

    /** Return if this track is active or not */
    bool isActive();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

protected slots:
    void slotTrackChanged(quint32 id);
signals:
    void itemClicked(TrackItem *);

private:
    QString m_name;
    int m_number;
    QFont m_font;
    bool m_isActive;
    Track *m_track;
};

/*********************************************************************
 * Sequence Item. Clickable and draggable object identifying a chaser
 *********************************************************************/
class SequenceItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    SequenceItem(Chaser *seq);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void setTimeScale(int val);

    void setTrackIndex(int idx);
    int getTrackIndex();

    /** Return a pointer to a Chaser associated to this item */
    Chaser *getChaser();

signals:
    void itemDropped(QGraphicsSceneMouseEvent *, SequenceItem *);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

protected slots:
    void slotSequenceChanged(quint32);

private:
    /** Calculate sequence width for paint() and boundingRect() */
    void calculateWidth();

private:
    QColor color;
    /** Reference to the actual Chaser object which holds the sequence steps */
    Chaser *m_chaser;
    /** width of the graphics object. Recalculated every time a chaser step  changes */
    int m_width;
    /** horizontal scale to adapt width to the current time line */
    int m_timeScale;
    /** track index this sequence belongs to */
    int m_trackIdx;
};

#endif
