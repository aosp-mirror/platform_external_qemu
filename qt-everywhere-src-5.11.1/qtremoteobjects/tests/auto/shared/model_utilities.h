/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QModelIndex>

namespace {

inline bool compareIndices(const QModelIndex &lhs, const QModelIndex &rhs)
{
    QModelIndex left = lhs;
    QModelIndex right = rhs;
    while (left.row() == right.row() && left.column() == right.column() && left.isValid() && right.isValid()) {
        left = left.parent();
        right = right.parent();
    }
    if (left.isValid() || right.isValid())
        return false;
    return true;
}

struct WaitForDataChanged
{
    struct IndexPair
    {
        QModelIndex topLeft;
        QModelIndex bottomRight;
    };

    WaitForDataChanged(const QVector<QModelIndex> &pending, QSignalSpy *spy) : m_pending(pending), m_spy(spy){}
    bool wait()
    {
        Q_ASSERT(m_spy);
        const int maxRuns = std::min(m_pending.size(), 100);
        int runs = 0;
        bool cancel = false;
        while (!cancel) {
            const int numSignals = m_spy->size();
            for (int i = 0; i < numSignals; ++i) {
                const QList<QVariant> &signal = m_spy->takeFirst();
                IndexPair pair = extractPair(signal);
                checkAndRemoveRange(pair.topLeft, pair.bottomRight);
                cancel = m_pending.isEmpty();
            }
            if (!cancel)
                m_spy->wait(50);
            ++runs;
            if (runs >= maxRuns)
                cancel = true;
        }
        return runs < maxRuns;
    }

    static IndexPair extractPair(const QList<QVariant> &signal)
    {
        IndexPair pair;
        if (signal.size() != 3)
            return pair;
        const bool matchingTypes = signal[0].type() == QVariant::nameToType("QModelIndex")
                                   && signal[1].type() == QVariant::nameToType("QModelIndex")
                                   && signal[2].type() == QVariant::nameToType("QVector<int>");
        if (!matchingTypes)
            return pair;
        const QModelIndex topLeft = signal[0].value<QModelIndex>();
        const QModelIndex bottomRight = signal[1].value<QModelIndex>();
        pair.topLeft = topLeft;
        pair.bottomRight = bottomRight;
        return pair;
    }

    void checkAndRemoveRange(const QModelIndex &topLeft, const QModelIndex &bottomRight)
    {
        QVERIFY(topLeft.parent() == bottomRight.parent());
        QVector<QModelIndex>  toRemove;
        for (int i = 0; i < m_pending.size(); ++i) {
            const QModelIndex &pending = m_pending.at(i);
            if (pending.isValid()  && compareIndices(pending.parent(), topLeft.parent())) {
                const bool fitLeft = topLeft.column() <= pending.column();
                const bool fitRight = bottomRight.column() >= pending.column();
                const bool fitTop = topLeft.row() <= pending.row();
                const bool fitBottom = bottomRight.row() >= pending.row();
                if (fitLeft && fitRight && fitTop && fitBottom)
                    toRemove.append(pending);
            }
        }
        foreach (const QModelIndex &index, toRemove) {
            const int ind = m_pending.indexOf(index);
            m_pending.remove(ind);
        }
    }

    QVector<QModelIndex> m_pending;
    QSignalSpy *m_spy;
};

} // namespace
