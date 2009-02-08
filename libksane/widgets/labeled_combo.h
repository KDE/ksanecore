/* ============================================================
 *
 * This file is part of the KDE project
 *
 * Date        : 2007-09-13
 * Description : Sane interface for KDE
 *
 * Copyright (C) 2007-2008 by Kare Sars <kare dot sars at iki dot fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================ */

#ifndef LABELED_COMBO_H
#define LABELED_COMBO_H

// Qt includes.
#include <QFrame>
#include <QLabel>
#include <QGridLayout>

// KDE includes
#include <KComboBox>

namespace KSaneIface
{

/**
 * A label and a combobox.
 */
class LabeledCombo : public QFrame
{
    Q_OBJECT

public:
   /**
    * create a label and combobox combination.
    * \param parent parent widget
    * \param label is the lext for the label
    * \param list a stringlist with values the list should contain.
    */
    LabeledCombo(QWidget *parent, const QString& label, const QStringList& list);

   /**
    * Set the label
    */
    void setLabelText(const QString &text);

    void widgetSizeHints(int *lab_w, int *cmb_w);
    void setColumnWidths(int lab_w, int cmb_w);

public Q_SLOTS:

    /**
     * Add string entries to the combobox
     */
    void addItems(const QStringList &list) { m_combo->addItems(list); }

    /**
     * Remove all string entries
     */
    void clear() { m_combo->clear(); }

   /**
    * If the given string can be found in the comobox, activate that entry.
    * If not, the entry is not changed.
    */
    void setCurrentText(const QString &);

   /**
    * set the current item of the combobox.
    */
    void setCurrentIndex(int);

   /**
    * add an icon for a string in the combobox
    * @param icon the icon to set.
    * @param str the string for which the pixmap should be set.
    */
    bool setIcon(const QIcon& icon, const QString& str);

private Q_SLOTS:

    void prActivated(const QString &);
    void prActivated(int);

Q_SIGNALS:

    void activated(int);
    void activated(const QString&);

private:

    QLabel      *m_label;
    KComboBox   *m_combo;
    QGridLayout *m_layout;
};

}  // NameSpace KSaneIface

#endif // LABELED_COMBO_H
