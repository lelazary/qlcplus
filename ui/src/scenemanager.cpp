/*
  Q Light Controller
  scenemanager.cpp

  Copyright (C) Massimo Callegari

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

#include <QInputDialog>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QScrollBar>
#include <QComboBox>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QDebug>

#include "multitrackview.h"
#include "sceneselection.h"
#include "scenemanager.h"
#include "chasereditor.h"
#include "scenerunner.h"
#include "sceneeditor.h"
#include "sceneitems.h"
#include "chaser.h"

SceneManager* SceneManager::s_instance = NULL;

SceneManager::SceneManager(QWidget* parent, Doc* doc)
    : QWidget(parent)
    , m_doc(doc)
    , m_show(NULL)
    , m_scene(NULL)
    , m_scene_editor(NULL)
    , m_sequence_editor(NULL)
    , m_splitter(NULL)
    , m_vsplitter(NULL)
    , m_showview(NULL)
    , m_runner(NULL)
    , is_playing(false)
    , m_toolbar(NULL)
    , m_showsCombo(NULL)
    , m_addShowAction(NULL)
    , m_addTrackAction(NULL)
    , m_addSequenceAction(NULL)
    , m_cloneAction(NULL)
    , m_deleteAction(NULL)
    , m_stopAction(NULL)
    , m_playAction(NULL)
{
    Q_ASSERT(s_instance == NULL);
    s_instance = this;

    Q_ASSERT(doc != NULL);

    new QVBoxLayout(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);

    initActions();
    initToolbar();

    m_splitter = new QSplitter(Qt::Vertical, this);
    layout()->addWidget(m_splitter);
    //initMultiTrackView();
    m_showview = new MultiTrackView();
    // add container for multitrack+chaser view
    QWidget* gcontainer = new QWidget(this);
    m_splitter->addWidget(gcontainer);
    gcontainer->setLayout(new QVBoxLayout);
    gcontainer->layout()->setContentsMargins(0, 0, 0, 0);
    //m_showview = new QGraphicsView(m_showscene);
    m_showview->setRenderHint(QPainter::Antialiasing);
    m_showview->setAcceptDrops(true);
    m_showview->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_showview->setBackgroundBrush(QBrush(QColor(88, 88, 88, 255), Qt::SolidPattern));
    connect(m_showview, SIGNAL(viewClicked ( QMouseEvent * )),
            this, SLOT(slotViewClicked( QMouseEvent * )));
    connect(m_showview, SIGNAL(sequenceMoved(SequenceItem *)),
            this, SLOT(slotSequenceMoved(SequenceItem*)));
    connect(m_showview, SIGNAL(timeChanged(quint32)),
            this, SLOT(slotUpdateTime(quint32)));
    connect(m_showview, SIGNAL(trackClicked(Track*)),
            this, SLOT(slotTrackClicked(Track*)));

    // split the multitrack view into two (left: tracks, right: chaser editor)
    m_vsplitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->widget(0)->layout()->addWidget(m_vsplitter);
    QWidget* mcontainer = new QWidget(this);
    mcontainer->setLayout(new QHBoxLayout);
    mcontainer->layout()->setContentsMargins(0, 0, 0, 0);
    m_vsplitter->addWidget(mcontainer);
    m_vsplitter->widget(0)->layout()->addWidget(m_showview);

    // add container for chaser editor
    QWidget* ccontainer = new QWidget(this);
    m_vsplitter->addWidget(ccontainer);
    ccontainer->setLayout(new QVBoxLayout);
    ccontainer->layout()->setContentsMargins(0, 0, 0, 0);

    // add container for scene editor
    QWidget* container = new QWidget(this);
    m_splitter->addWidget(container);
    container->setLayout(new QVBoxLayout);
    container->layout()->setContentsMargins(0, 0, 0, 0);
    
    //connect(m_doc, SIGNAL(modeChanged(Doc::Mode)), this, SLOT(slotModeChanged()));
    connect(m_doc, SIGNAL(clearing()), this, SLOT(slotDocClearing()));
    connect(m_doc, SIGNAL(functionChanged(quint32)), this, SLOT(slotFunctionChanged(quint32)));
}

SceneManager::~SceneManager()
{
    SceneManager::s_instance = NULL;
}

SceneManager* SceneManager::instance()
{
    return s_instance;
}

void SceneManager::initActions()
{
    /* Manage actions */
    m_addShowAction = new QAction(QIcon(":/show.png"),
                                   tr("New s&how"), this);
    m_addShowAction->setShortcut(QKeySequence("CTRL+H"));
    connect(m_addShowAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddShow()));

    m_addTrackAction = new QAction(QIcon(":/track.png"),
                                   tr("New &track"), this);
    m_addTrackAction->setShortcut(QKeySequence("CTRL+T"));
    connect(m_addTrackAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddTrack()));

    m_addSequenceAction = new QAction(QIcon(":/sequence.png"),
                                    tr("New s&equence"), this);
    m_addSequenceAction->setShortcut(QKeySequence("CTRL+E"));
    connect(m_addSequenceAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddSequence()));

    /* Edit actions */
    m_cloneAction = new QAction(QIcon(":/editcopy.png"),
                                tr("&Clone"), this);
    m_cloneAction->setShortcut(QKeySequence("CTRL+C"));
    connect(m_cloneAction, SIGNAL(triggered(bool)),
            this, SLOT(slotClone()));

    m_deleteAction = new QAction(QIcon(":/editdelete.png"),
                                 tr("&Delete"), this);
    m_deleteAction->setShortcut(QKeySequence("Delete"));
    connect(m_deleteAction, SIGNAL(triggered(bool)),
            this, SLOT(slotDelete()));
    m_deleteAction->setEnabled(false);

    m_stopAction = new QAction(QIcon(":/design.png"),  /* @todo re-used icon...to be changed */
                                 tr("St&op"), this);
    m_stopAction->setShortcut(QKeySequence("CTRL+O"));
    connect(m_stopAction, SIGNAL(triggered(bool)),
            this, SLOT(slotStopPlayback()));

    m_playAction = new QAction(QIcon(":/operate.png"), /* @todo re-used icon...to be changed */
                                 tr("&Play"), this);
    m_playAction->setShortcut(QKeySequence("SPACE"));
    connect(m_playAction, SIGNAL(triggered(bool)),
            this, SLOT(slotStartPlayback()));
}

void SceneManager::initToolbar()
{
    // Add a toolbar to the dock area
    m_toolbar = new QToolBar("Show Manager", this);
    m_toolbar->setFloatable(false);
    m_toolbar->setMovable(false);
    layout()->addWidget(m_toolbar);
    m_toolbar->addAction(m_addShowAction);
    m_showsCombo = new QComboBox();
    m_showsCombo->setFixedWidth(180);
    connect(m_showsCombo, SIGNAL(currentIndexChanged(int)),this, SLOT(slotShowsComboChanged(int)));
    m_toolbar->addWidget(m_showsCombo);
    m_toolbar->addSeparator();

    m_toolbar->addAction(m_addTrackAction);
    m_toolbar->addAction(m_addSequenceAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_cloneAction);
    m_toolbar->addAction(m_deleteAction);
    m_toolbar->addSeparator();

    // Time label and playback buttons
    m_timeLabel = new QLabel("00:00:00:000");
    m_timeLabel->setFixedWidth(150);
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    QFont timeFont = QApplication::font();
    timeFont.setBold(true);
    timeFont.setPixelSize(20);
    m_timeLabel->setFont(timeFont);
    m_toolbar->addWidget(m_timeLabel);
    m_toolbar->addSeparator();

    m_toolbar->addAction(m_stopAction);
    m_toolbar->addAction(m_playAction);
}

/*********************************************************************
 * Shows combo
 *********************************************************************/
void SceneManager::updateShowsCombo()
{
    int newIndex = 0;
    m_showsCombo->clear();
    foreach (Function* function, m_doc->functions())
    {
        if (function->type() == Function::Show)
        {
            m_showsCombo->addItem(function->name(), QVariant(function->id()));
            if (m_show != NULL && m_show->id() != function->id())
                newIndex++;
        }
    }
    if (m_showsCombo->count() > 0)
    {
        m_showsCombo->setCurrentIndex(newIndex);
        m_addTrackAction->setEnabled(true);
    }
    else
    {
        m_addTrackAction->setEnabled(false);
        m_addSequenceAction->setEnabled(false);
    }
}

void SceneManager::slotShowsComboChanged(int idx)
{
    qDebug() << Q_FUNC_INFO << "Idx: " << idx;
    updateMultiTrackView();
}

void SceneManager::showSceneEditor(Scene *scene)
{
    if (m_scene_editor != NULL)
    {
        m_splitter->widget(1)->layout()->removeWidget(m_scene_editor);
        m_scene_editor->deleteLater();
        m_scene_editor = NULL;
    }

    m_scene_editor = new SceneEditor(m_splitter->widget(1), scene, m_doc, false);
    if (m_scene_editor != NULL)
    {
        connect(this, SIGNAL(functionManagerActive(bool)),
                    m_scene_editor, SLOT(slotFunctionManagerActive(bool)));
        m_splitter->widget(1)->layout()->addWidget(m_scene_editor);
        m_splitter->widget(1)->show();
        m_scene_editor->show();
    }
}

void SceneManager::showSequenceEditor(Chaser *chaser)
{
    if (m_sequence_editor != NULL)
    {
        m_vsplitter->widget(1)->layout()->removeWidget(m_sequence_editor);
        m_sequence_editor->deleteLater();
        m_sequence_editor = NULL;
    }

    m_sequence_editor = new ChaserEditor(m_vsplitter->widget(1), chaser, m_doc);
    if (m_sequence_editor != NULL)
    {
        m_vsplitter->widget(1)->layout()->addWidget(m_sequence_editor);
        /** Signal from chaser editor to scene editor. When a step is clicked apply values immediately */
        connect(m_sequence_editor, SIGNAL(applyValues(QList<SceneValue>&)),
                m_scene_editor, SLOT(slotSetSceneValues(QList <SceneValue>&)));
        /** Signal from scene editor to chaser editor. When a fixture value is changed, update the selected chaser step */
        connect(m_scene_editor, SIGNAL(fixtureValueChanged(SceneValue)),
                m_sequence_editor, SLOT(slotUpdateCurrentStep(SceneValue)));
        m_vsplitter->widget(1)->show();
        m_sequence_editor->show();
    }
}

void SceneManager::slotAddShow()
{
    m_show = new Show(m_doc);
    Function *f = qobject_cast<Function*>(m_show);
    if (m_doc->addFunction(f) == true)
    {
        f->setName(QString("%1 %2").arg(tr("New Show")).arg(f->id()));
        bool ok;
        QString text = QInputDialog::getText(this, tr("Show name setup"),
                                             tr("Show name:"), QLineEdit::Normal,
                                             f->name(), &ok);
        if (ok && !text.isEmpty())
            m_show->setName(text);
        updateShowsCombo();
    }
}

void SceneManager::slotAddTrack()
{
    if (m_show == NULL)
        return;

    QList <quint32> disabledIDs;
    if (m_show->tracks().count() > 0)
    {
        /** Add already Scene IDs already assigned in this Show */
        foreach (Track *track, m_show->tracks())
            disabledIDs.append(track->getSceneID());
    }

    SceneSelection ss(this, m_doc);
    ss.setDisabledScenes(disabledIDs);
    if (ss.exec() == QDialog::Accepted)
    {
        /* Do we have to create a new Scene ? */
        if (ss.getSelectedID() == Scene::invalidId())
        {
            m_scene = new Scene(m_doc);
            Function *f = qobject_cast<Function*>(m_scene);
            if (m_doc->addFunction(f) == true)
                f->setName(QString("%1 %2").arg(tr("New Scene")).arg(f->id()));
        }
        else
        {
            m_scene = qobject_cast<Scene*>(m_doc->function(ss.getSelectedID()));
        }
        Track* newTrack = new Track(m_scene->id());
        newTrack->setName(m_scene->name());
        m_show->addTrack(newTrack);
        showSceneEditor(m_scene);
        m_showview->addTrack(newTrack);
        m_addSequenceAction->setEnabled(true);
    }
}

void SceneManager::slotAddSequence()
{
    Function* f = new Chaser(m_doc);
    Chaser *chaser = qobject_cast<Chaser*> (f);
    quint32 sceneID = m_scene->id();
    chaser->enableSequenceMode(sceneID);
    chaser->setRunOrder(Function::SingleShot);
    Scene *boundScene = qobject_cast<Scene*>(m_doc->function(sceneID));
    boundScene->setChildrenFlag(true);
    if (m_doc->addFunction(f) == true)
    {
        f->setName(QString("%1 %2").arg(tr("New Sequence")).arg(f->id()));
        showSequenceEditor(chaser);
        Track *track = m_show->getTrackFromSceneID(m_scene->id());
        track->addSequence(chaser);
        m_showview->addSequence(chaser);
    }
}

void SceneManager::slotClone()
{
}

void SceneManager::slotDelete()
{
    quint32 deleteID = m_showview->deleteSelectedSequence();
    if (deleteID != Function::invalidId())
    {
        m_doc->deleteFunction(deleteID);
        if (m_sequence_editor != NULL)
        {
            m_vsplitter->widget(1)->layout()->removeWidget(m_sequence_editor);
            m_sequence_editor->deleteLater();
            m_sequence_editor = NULL;
        }
        m_deleteAction->setEnabled(false);
    }
}

void SceneManager::slotStopPlayback()
{
    if (m_runner != NULL)
    {
        m_runner->stop();
        delete m_runner;
        m_runner = NULL;
    }
    m_showview->rewindCursor();
    m_timeLabel->setText("00:00:00.000");
}

void SceneManager::slotStartPlayback()
{
    if (m_showsCombo->count() == 0)
        return;
    if (m_runner != NULL)
    {
        m_runner->stop();
        delete m_runner;
    }

    m_runner = new SceneRunner(m_doc, m_showsCombo->itemData(m_showsCombo->currentIndex()).toUInt());
    connect(m_runner, SIGNAL(timeChanged(quint32)), this, SLOT(slotupdateTimeAndCursor(quint32)));
    m_runner->start();
}

void SceneManager::slotViewClicked(QMouseEvent *event)
{
    qDebug() << Q_FUNC_INFO << "View clicked at pos: " << event->pos().x() << event->pos().y();
    if (m_sequence_editor != NULL)
    {
        m_vsplitter->widget(1)->layout()->removeWidget(m_sequence_editor);
        m_sequence_editor->deleteLater();
        m_sequence_editor = NULL;
    }
    m_deleteAction->setEnabled(false);
}

void SceneManager::slotSequenceMoved(SequenceItem *item)
{
    qDebug() << Q_FUNC_INFO << "Sequence moved.........";
    Chaser *chaser = item->getChaser();
    if (chaser == NULL)
        return;
    showSequenceEditor(chaser);
    /* Check if scene has changed */
    quint32 newSceneID = chaser->getBoundedSceneID();
    if (newSceneID != m_scene->id())
    {
        Function *f = m_doc->function(newSceneID);
        if (f == NULL)
            return;
        m_scene = qobject_cast<Scene*>(f);
        showSceneEditor(m_scene);
        /* activate the new track */
        Track *track = m_show->getTrackFromSceneID(newSceneID);
        m_showview->activateTrack(track);
    }
    m_deleteAction->setEnabled(true);
}

void SceneManager::slotupdateTimeAndCursor(quint32 msec_time)
{
    //qDebug() << Q_FUNC_INFO << "time: " << msec_time;
    slotUpdateTime(msec_time);
    m_showview->moveCursor(msec_time);
}

void SceneManager::slotUpdateTime(quint32 msec_time)
{
    QTime tmpTime = QTime(0, 0, 0, 0).addMSecs(msec_time);

    m_timeLabel->setText(tmpTime.toString("hh:mm:ss.zzz"));
}

void SceneManager::slotTrackClicked(Track *track)
{
    Function *f = m_doc->function(track->getSceneID());
    if (f == NULL)
        return;
    m_scene = qobject_cast<Scene*>(f);
    showSceneEditor(m_scene);
}

void SceneManager::slotDocClearing()
{
    if (m_showview != NULL)
        m_showview->resetView();

    if (m_sequence_editor != NULL)
    {
        m_vsplitter->widget(1)->layout()->removeWidget(m_sequence_editor);
        m_sequence_editor->deleteLater();
        m_sequence_editor = NULL;
    }

    if (m_scene_editor != NULL)
    {
        m_splitter->widget(1)->layout()->removeWidget(m_scene_editor);
        m_scene_editor->deleteLater();
        m_scene_editor = NULL;
    }

    m_addTrackAction->setEnabled(false);
    m_addSequenceAction->setEnabled(false);
    m_deleteAction->setEnabled(false);
}

void SceneManager::slotFunctionChanged(quint32 id)
{
    Function* function = m_doc->function(id);
    if (function == NULL)
        return;

    if (function->type() == Function::Scene)
    {
        Track *trk = m_show->getTrackFromSceneID(id);
        if (trk != NULL)
            trk->setName(function->name());
    }
}

void SceneManager::updateMultiTrackView()
{
    m_showview->resetView();
    /* first of all get the ID of the selected scene */
    int idx = m_showsCombo->currentIndex();
    if (idx == -1)
        return;
    quint32 showID = m_showsCombo->itemData(idx).toUInt();

    bool first = true;
    /* create a SequenceItem for each Chaser that is a sequence and is bound to the scene ID */
    m_show = qobject_cast<Show *>(m_doc->function(showID));
    if (m_show == NULL)
    {
        qDebug() << Q_FUNC_INFO << "Invalid show !";
        return;
    }

    Scene *firstScene = NULL;

    foreach(Track *track, m_show->tracks())
    {
        m_scene = qobject_cast<Scene*>(m_doc->function(track->getSceneID()));
        if (m_scene == NULL)
        {
            qDebug() << Q_FUNC_INFO << "Invalid scene !";
            continue;
        }
        if (first == true)
        {
            firstScene = m_scene;
            first = false;
        }
        m_showview->addTrack(track);
        m_addSequenceAction->setEnabled(true);

        foreach(quint32 id, track->sequences())
        {
            Chaser *chaser = qobject_cast<Chaser*>(m_doc->function(id));
            m_showview->addSequence(chaser);
        }
    }
    /** Set first track active */
    Track *firstTrack = m_show->getTrackFromSceneID(firstScene->id());
    m_scene = firstScene;
    m_showview->activateTrack(firstTrack);
    showSceneEditor(m_scene);

}

void SceneManager::showEvent(QShowEvent* ev)
{
    qDebug() << Q_FUNC_INFO;
    emit functionManagerActive(true);
    QWidget::showEvent(ev);
    m_showview->show();
    m_showview->horizontalScrollBar()->setSliderPosition(0);
    updateShowsCombo();
}

void SceneManager::hideEvent(QHideEvent* ev)
{
    qDebug() << Q_FUNC_INFO;
    emit functionManagerActive(false);
    QWidget::hideEvent(ev);
    
    if (m_scene_editor != NULL)
        m_scene_editor->deleteLater();
    m_scene_editor = NULL;
}
