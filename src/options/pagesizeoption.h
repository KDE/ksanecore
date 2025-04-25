/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_PAGESIZE_OPTION_H
#define KSANE_PAGESIZE_OPTION_H

#include <QList>
#include <QSizeF>

#include "baseoption.h"

namespace KSaneCore
{

static const QString PageSizeOptionName = QStringLiteral("KSane::PageSize");

class PageSizeOption : public BaseOption
{
    Q_OBJECT

public:
    PageSizeOption(BaseOption *optionTopLeftX,
                   BaseOption *optionTopLeftY,
                   BaseOption *optionBottomRightX,
                   BaseOption *optionBottomRightY,
                   BaseOption *optionResolution,
                   BaseOption *optionPageWidth,
                   BaseOption *optionPageHeight);

    QVariant value() const override;
    QString valueAsString() const override;

    Option::OptionState state() const override;
    QString name() const override;
    QString title() const override;
    QString description() const override;
    QVariantList valueList() const override;
    QVariantList internalValueList() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;
    void storeOptions();
    void restoreOptions();
    void computePageSizes();

private Q_SLOTS:
    void optionTopLeftXUpdated();
    void optionTopLeftYUpdated();
    void optionBottomRightXUpdated();
    void optionBottomRightYUpdated();

private:
    struct PageSizeProperties {
        QString name;
        QSizeF pageSize;
        QSizeF wiggleRoom;
    };

    double ensureMilliMeter(BaseOption *option, double value);

    BaseOption *m_optionTopLeftX;
    BaseOption *m_optionTopLeftY;
    BaseOption *m_optionBottomRightX;
    BaseOption *m_optionBottomRightY;
    BaseOption *m_optionResolution;
    BaseOption *m_optionPageWidth;
    BaseOption *m_optionPageHeight;
    int m_currentIndex = -1;
    Option::OptionState m_state = Option::StateDisabled;
    QList<PageSizeProperties> m_availableSizes;
    double m_previousCoordinates[4];
};

} // namespace KSaneCore

#endif // KSANE_PAGESIZE_OPTION_H
