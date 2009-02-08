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

#ifndef LABELED_CHECKBOX_H
#define LABELED_CHECKBOX_H

// Qt includes.
#include <QFrame>
#include <QCheckBox>
#include <QGridLayout>

namespace KSaneIface
{

/**
  *@author Kåre Särs
  */

/**
 * A wrapper for a checkbox
 */
class LabeledCheckbox : public QFrame
{
    Q_OBJECT

public:

   /**
    * Create the checkbox.
    *
    * \param parent parent widget
    * \param text is the text describing the checkbox.
    */
    LabeledCheckbox(QWidget *parent, const QString& text);
    ~LabeledCheckbox();
    void setChecked(bool);
    void widgetSizeHints(int *lab_w, int *chb_w);
    void setColumnWidths(int lab_w, int rest_w);

private Q_SLOTS:

    void prToggled(bool);

Q_SIGNALS:

    void toggled(bool);

private:

    QCheckBox *chbx;
    QGridLayout *layout;
};

}  // NameSpace KSaneIface

#endif // LABELED_CHECKBOX_H
