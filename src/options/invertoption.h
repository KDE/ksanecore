/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 * 
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_INVERT_OPTION_H
#define KSANE_INVERT_OPTION_H

#include "baseoption.h"

namespace KSane
{

static const QString InvertColorsOptionName = QStringLiteral("KSane::InvertColors");    
    
class InvertOption : public BaseOption
{
    Q_OBJECT

public:
    InvertOption();

    QVariant value() const override;
    QString valueAsString() const override;
    
    CoreOption::OptionState state() const override;
    QString name() const override;
    QString title() const override;
    QString description() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    bool m_checked = false;
};  

} // namespace KSane

#endif // KSANE_INVERT_OPTION_H
