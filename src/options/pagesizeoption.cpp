/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "pagesizeoption.h"

#include <QPageSize>
#include <QSizeF>

#include <ksanecore_debug.h>

static constexpr int PageSizeWiggleRoom = 2; // in mm

namespace KSaneCore
{

PageSizeOption::PageSizeOption(BaseOption *optionTopLeftX,
                               BaseOption *optionTopLeftY,
                               BaseOption *optionBottomRightX,
                               BaseOption *optionBottomRightY,
                               BaseOption *optionResolution,
                               BaseOption *optionPageWidth,
                               BaseOption *optionPageHeight)
    : BaseOption()
{
    if (optionTopLeftX == nullptr || optionTopLeftY == nullptr || optionBottomRightX == nullptr || optionBottomRightY == nullptr) {
        m_optionType = Option::TypeDetectFail;
        return;
    }

    connect(optionTopLeftX, &BaseOption::valueChanged, this, &PageSizeOption::optionTopLeftXUpdated);
    connect(optionTopLeftY, &BaseOption::valueChanged, this, &PageSizeOption::optionTopLeftYUpdated);
    connect(optionBottomRightX, &BaseOption::valueChanged, this, &PageSizeOption::optionBottomRightXUpdated);
    connect(optionBottomRightY, &BaseOption::valueChanged, this, &PageSizeOption::optionBottomRightYUpdated);

    m_optionTopLeftX = optionTopLeftX;
    m_optionTopLeftY = optionTopLeftY;
    m_optionBottomRightX = optionBottomRightX;
    m_optionBottomRightY = optionBottomRightY;
    m_optionResolution = optionResolution;
    m_optionPageWidth = optionPageWidth;
    m_optionPageHeight = optionPageHeight;

    m_optionType = Option::TypeValueList;

    computePageSizes();
}

bool PageSizeOption::setValue(const QVariant &value)
{
    if (value.userType() == QMetaType::QString) {
        QString newValue = value.toString();
        if (newValue == m_availableSizesListNames.at(m_currentIndex)) {
            return true;
        }
        for (int i = 0; i < m_availableSizesListNames.size(); i++) {
            QString sizeEntry = m_availableSizesListNames.at(i).toString();
            if (sizeEntry == newValue) {
                m_currentIndex = i;

                if (i != 0) {
                    const auto size = m_availableSizesList.at(i);
                    if (m_optionPageWidth != nullptr && m_optionPageHeight != nullptr) {
                        m_optionPageWidth->setValue(size.width());
                        m_optionPageHeight->setValue(size.height());
                    }
                    m_optionTopLeftX->setValue(0);
                    m_optionTopLeftY->setValue(0);
                    m_optionBottomRightX->setValue(size.width());
                    m_optionBottomRightY->setValue(size.height());
                }
                Q_EMIT valueChanged(sizeEntry);
                return true;
            }
        }
    }
    return false;
}

QVariant PageSizeOption::value() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_availableSizesListNames.size()) {
        return m_availableSizesListNames.at(m_currentIndex);
    } else {
        return QVariant();
    }
}

QString PageSizeOption::valueAsString() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_availableSizesListNames.size()) {
        return m_availableSizesListNames.at(m_currentIndex).toString();
    } else {
        return QString();
    }
}

QVariantList PageSizeOption::valueList() const
{
    return m_availableSizesListNames;
}

QVariantList PageSizeOption::internalValueList() const
{
    return m_availableSizesListNames;
}

Option::OptionState PageSizeOption::state() const
{
    return m_state;
}

QString PageSizeOption::name() const
{
    return PageSizeOptionName;
}

QString PageSizeOption::title() const
{
    return i18n("Scan area size");
}

QString PageSizeOption::description() const
{
    return i18n("Select a predefined page size for the scanning area.");
}

void PageSizeOption::optionTopLeftXUpdated()
{
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size() && m_optionTopLeftX->value().toDouble() != 0) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

void PageSizeOption::optionTopLeftYUpdated()
{
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size() && m_optionTopLeftY->value().toDouble() != 0) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

void PageSizeOption::optionBottomRightXUpdated()
{
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size()
        && ensureMilliMeter(m_optionBottomRightX, m_optionBottomRightX->value().toDouble()) != m_availableSizesList.at(m_currentIndex).width()) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

void PageSizeOption::optionBottomRightYUpdated()
{
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size()
        && ensureMilliMeter(m_optionBottomRightY, m_optionBottomRightY->value().toDouble()) != m_availableSizesList.at(m_currentIndex).height()) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

double PageSizeOption::ensureMilliMeter(BaseOption *option, double value)
{
    // convert if necessary with current DPI if available
    if (option->valueUnit() == Option::UnitPixel && m_optionResolution != nullptr) {
        double dpi = m_optionResolution->value().toDouble();
        if (dpi > 1) {
            return value / (dpi / 25.4);
        }
    }
    return value;
}

void PageSizeOption::storeOptions()
{
    m_previousCoordinates[0] = m_optionTopLeftX->value().toDouble();
    m_previousCoordinates[1] = m_optionTopLeftY->value().toDouble();
    m_previousCoordinates[2] = m_optionBottomRightX->value().toDouble();
    m_previousCoordinates[3] = m_optionBottomRightY->value().toDouble();
}

void PageSizeOption::restoreOptions()
{
    computePageSizes();
    Q_EMIT optionReloaded();

    m_optionTopLeftX->setValue(m_previousCoordinates[0]);
    m_optionTopLeftY->setValue(m_previousCoordinates[1]);
    m_optionBottomRightX->setValue(m_previousCoordinates[2]);
    m_optionBottomRightY->setValue(m_previousCoordinates[3]);

    int newIndex = 0;
    if (m_optionTopLeftX->value().toDouble() == 0 && m_optionTopLeftY->value().toDouble() == 0) {
        QSizeF currentSize = QSizeF(ensureMilliMeter(m_optionBottomRightX, m_optionBottomRightX->value().toDouble()),
                                    ensureMilliMeter(m_optionBottomRightY, m_optionBottomRightY->value().toDouble()));

        for (int i = 0; i < m_availableSizesList.count(); i++) {
            if (qFuzzyCompare(currentSize.height(), m_availableSizesList.at(i).height())
                && qFuzzyCompare(currentSize.width(), m_availableSizesList.at(i).width())) {
                newIndex = i;
            }
        }
    }
    if (newIndex != m_currentIndex) {
        m_currentIndex = newIndex;
        Q_EMIT valueChanged(m_availableSizesListNames.at(m_currentIndex));
    }
}

void PageSizeOption::computePageSizes()
{
    /* some SANE backends set the maximum value of bottom right X and Y to the current page width and height values
     * set current values of these option to maximum if available, such that we detect possible page sizes correctly
     * see https://gitlab.com/sane-project/backends/-/issues/730 and https://bugs.kde.org/show_bug.cgi?id=476838 */
    if (m_optionPageWidth != nullptr && m_optionPageHeight != nullptr) {
        m_optionPageHeight->storeCurrentData();
        m_optionPageWidth->storeCurrentData();
        m_optionPageHeight->setValue(m_optionPageHeight->maximumValue());
        m_optionPageWidth->setValue(m_optionPageWidth->maximumValue());
    }

    const QList<QPageSize::PageSizeId> possibleSizesList = {
        QPageSize::A3,        QPageSize::A4,    QPageSize::A5,     QPageSize::A6,    QPageSize::Letter, QPageSize::Legal,   QPageSize::Tabloid,
        QPageSize::B3,        QPageSize::B4,    QPageSize::B5,     QPageSize::B6,    QPageSize::C5E,    QPageSize::Comm10E, QPageSize::DLE,
        QPageSize::Executive, QPageSize::Folio, QPageSize::Ledger, QPageSize::JisB3, QPageSize::JisB4,  QPageSize::JisB5,   QPageSize::JisB6,
    };

    m_availableSizesList.clear();
    m_availableSizesListNames.clear();

    m_availableSizesList << QPageSize::size(QPageSize::Custom, QPageSize::Millimeter);
    m_availableSizesListNames << QPageSize::name(QPageSize::Custom);

    double maxScannerWidth = ensureMilliMeter(m_optionBottomRightX, m_optionBottomRightX->maximumValue().toDouble());
    double maxScannerHeight = ensureMilliMeter(m_optionBottomRightY, m_optionBottomRightY->maximumValue().toDouble());

    // Add portrait page sizes
    for (const auto sizeCode : possibleSizesList) {
        QSizeF size = QPageSize::size(sizeCode, QPageSize::Millimeter);
        if (size.width() - PageSizeWiggleRoom > maxScannerWidth) {
            continue;
        }
        if (size.height() - PageSizeWiggleRoom > maxScannerHeight) {
            continue;
        }
        m_availableSizesList << size;
        m_availableSizesListNames << QPageSize::name(sizeCode);
    }

    // Add landscape page sizes
    for (const auto sizeCode : possibleSizesList) {
        QSizeF size = QPageSize::size(sizeCode, QPageSize::Millimeter);
        size.transpose();
        if (size.width() - PageSizeWiggleRoom > maxScannerWidth) {
            continue;
        }
        if (size.height() - PageSizeWiggleRoom > maxScannerHeight) {
            continue;
        }
        m_availableSizesList << size;
        m_availableSizesListNames << i18nc("Page size landscape", "Landscape %1", QPageSize::name(sizeCode));
    }

    // Set custom as current
    m_currentIndex = 0;
    if (m_availableSizesList.count() > 1) {
        m_state = Option::StateActive;
    } else {
        m_state = Option::StateHidden;
    }

    if (m_optionPageWidth != nullptr && m_optionPageHeight != nullptr) {
        m_optionPageHeight->restoreSavedData();
        m_optionPageWidth->restoreSavedData();
    }
}

} // namespace KSaneCore

#include "moc_pagesizeoption.cpp"
