/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_PAGESIZE_OPTION_H
#define KSANE_PAGESIZE_OPTION_H

#include <QList>

#include "baseoption.h"

namespace KSaneCore
{

static const QString PageSizeOptionName = QStringLiteral("KSane::PageSize");

class PageSizeOption : public BaseOption
{
    Q_OBJECT

public:
    PageSizeOption(BaseOption *m_optionTopLeftX,
                   BaseOption *m_optionTopLeftY,
                   BaseOption *m_optionBottomRightX,
                   BaseOption *m_optionBottomRightY,
                   BaseOption *m_optionResolution);

    QVariant value() const override;
    QString valueAsString() const override;

    Option::OptionState state() const override;
    QString name() const override;
    QString title() const override;
    QString description() const override;
    QVariantList valueList() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private Q_SLOTS:
    void optionTopLeftXUpdated();
    void optionTopLeftYUpdated();
    void optionBottomRightXUpdated();
    void optionBottomRightYUpdated();

private:
    double ensureMilliMeter(BaseOption *option, double value);

    BaseOption *m_optionTopLeftX;
    BaseOption *m_optionTopLeftY;
    BaseOption *m_optionBottomRightX;
    BaseOption *m_optionBottomRightY;
    BaseOption *m_optionResolution;
    int m_currentIndex = -1;
    Option::OptionState m_state = Option::StateDisabled;
    QVariantList m_availableSizesListNames;
    QList<QSizeF> m_availableSizesList;
};

} // namespace KSaneCore

#endif // KSANE_PAGESIZE_OPTION_H
