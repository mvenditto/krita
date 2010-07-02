/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_simple_update_queue_test.h"
#include <qtest_kde.h>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include "kis_merge_walker.h"
#include "kis_simple_update_queue.h"

void KisSimpleUpdateQueueTest::testJobProcessing()
{
    KisTestableUpdaterContext context(2);

    QRect imageRect(0,0,200,200);

    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, imageRect.width(), imageRect.height(), cs, "merge test");

    KisPaintLayerSP paintLayer = new KisPaintLayer(image, "test", OPACITY_OPAQUE_U8);

    image->lock();
    image->addNode(paintLayer);
    image->unlock();

    QRect dirtyRect1(0,0,50,100);
    KisBaseRectsWalkerSP walker1 = new KisMergeWalker(imageRect);
    walker1->collectRects(paintLayer, dirtyRect1);

    QRect dirtyRect2(0,0,100,100);
    KisBaseRectsWalkerSP walker2 = new KisMergeWalker(imageRect);
    walker2->collectRects(paintLayer, dirtyRect2);

    QRect dirtyRect3(50,0,50,100);
    KisBaseRectsWalkerSP walker3 = new KisMergeWalker(imageRect);
    walker3->collectRects(paintLayer, dirtyRect3);

    QRect dirtyRect4(150,150,50,50);
    KisBaseRectsWalkerSP walker4 = new KisMergeWalker(imageRect);
    walker4->collectRects(paintLayer, dirtyRect4);

    QVector<KisUpdateJobItem*> jobs;
    KisWalkersList walkersList;

    /**
     * Process the queue and look what has been added into
     * the updater context
     */

    KisTestableSimpleUpdateQueue queue;

    queue.addJob(walker1);
    queue.addJob(walker2);
    queue.addJob(walker3);
    queue.addJob(walker4);

    queue.processQueue(context);

    jobs = context.getJobs();

    QCOMPARE(jobs[0]->walker(), walker1);
    QCOMPARE(jobs[1]->walker(), walker3);
    QCOMPARE(jobs.size(), 2);

    walkersList = queue.getWalkersList();

    QCOMPARE(walkersList.size(), 2);
    QCOMPARE(walkersList[0], walker2);
    QCOMPARE(walkersList[1], walker4);


    /**
     * Test blocking the process
     */

    context.clear();

    queue.blockProcessing(context);

    queue.addJob(walker1);
    queue.addJob(walker2);
    queue.addJob(walker3);
    queue.addJob(walker4);

    jobs = context.getJobs();
    QCOMPARE(jobs[0]->walker(), KisBaseRectsWalkerSP(0));
    QCOMPARE(jobs[1]->walker(), KisBaseRectsWalkerSP(0));

    queue.startProcessing(context);

    jobs = context.getJobs();

    QCOMPARE(jobs[0]->walker(), walker2);
    QCOMPARE(jobs[1]->walker(), walker4);
}

void KisSimpleUpdateQueueTest::testOptimization()
{
    QRect imageRect(0,0,200,200);

    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, imageRect.width(), imageRect.height(), cs, "merge test");

    KisPaintLayerSP paintLayer = new KisPaintLayer(image, "test", OPACITY_OPAQUE_U8);

    image->lock();
    image->addNode(paintLayer);
    image->unlock();

    QRect dirtyRect1(0,0,50,100);
    KisBaseRectsWalkerSP walker1 = new KisMergeWalker(imageRect);
    walker1->collectRects(paintLayer, dirtyRect1);

    QRect dirtyRect2(0,0,100,100);
    KisBaseRectsWalkerSP walker2 = new KisMergeWalker(imageRect);
    walker2->collectRects(paintLayer, dirtyRect2);

    QRect dirtyRect3(50,0,50,100);
    KisBaseRectsWalkerSP walker3 = new KisMergeWalker(imageRect);
    walker3->collectRects(paintLayer, dirtyRect3);

    QRect dirtyRect4(150,150,50,50);
    KisBaseRectsWalkerSP walker4 = new KisMergeWalker(imageRect);
    walker4->collectRects(paintLayer, dirtyRect4);

    KisTestableSimpleUpdateQueue queue;
    KisWalkersList& walkersList = queue.getWalkersList();

    queue.addJob(walker1);
    queue.addJob(walker2);
    queue.addJob(walker3);
    queue.addJob(walker4);

    QCOMPARE(walkersList.size(), 4);
    QCOMPARE(walkersList[0], walker1);
    QCOMPARE(walkersList[1], walker2);
    QCOMPARE(walkersList[2], walker3);
    QCOMPARE(walkersList[3], walker4);


    queue.optimize();

    QCOMPARE(walkersList.size(), 2);
    QCOMPARE(walkersList[0], walker1);
    QCOMPARE(walkersList[1], walker4);

    QCOMPARE(walkersList[0]->requestedRect(), QRect(0,0,100,100));

}

QTEST_KDEMAIN(KisSimpleUpdateQueueTest, NoGUI)
#include "kis_simple_update_queue_test.moc"

