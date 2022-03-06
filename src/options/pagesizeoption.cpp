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

namespace KSane
{

PageSizeOption::PageSizeOption(BaseOption *optionTopLeftX, BaseOption *optionTopLeftY,
                        BaseOption *optionBottomRightX, BaseOption *optionBottomRightY,
                        BaseOption *optionResolution) : BaseOption()
{
    if (optionTopLeftX == nullptr || optionTopLeftY == nullptr || 
        optionBottomRightX == nullptr || optionBottomRightY == nullptr) {
        
        m_optionType = CoreOption::TypeDetectFail;
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
    
    const QList<QPageSize::PageSizeId> possibleSizesList = {
        QPageSize::A3,
        QPageSize::A4,
        QPageSize::A5,
        QPageSize::A6,
        QPageSize::Letter,
        QPageSize::Legal,
        QPageSize::Tabloid,
        QPageSize::B3,
        QPageSize::B4,
        QPageSize::B5,
        QPageSize::B6,
        QPageSize::C5E,
        QPageSize::Comm10E,
        QPageSize::DLE,
        QPageSize::Executive,
        QPageSize::Folio,
        QPageSize::Ledger,
        QPageSize::JisB3,
        QPageSize::JisB4,
        QPageSize::JisB5,
        QPageSize::JisB6,
    };
    
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
        m_state = CoreOption::StateActive;
    } else {
        m_state = CoreOption::StateHidden;
    }    
    m_optionType = CoreOption::TypeValueList;
}

bool PageSizeOption::setValue(const QVariant &value)
{
    if (static_cast<QMetaType::Type>(value.type()) == QMetaType::QString) {
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

CoreOption::OptionState PageSizeOption::state() const
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
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size() 
        && m_optionTopLeftY->value().toDouble() != 0 ) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

void PageSizeOption::optionTopLeftYUpdated()
{
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size() 
        && m_optionTopLeftY->value().toDouble() != 0 ) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

void PageSizeOption::optionBottomRightXUpdated()
{  
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size() 
        && ensureMilliMeter(m_optionBottomRightX, m_optionBottomRightX->value().toDouble())!= m_availableSizesList.at(m_currentIndex).width() ) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

void PageSizeOption::optionBottomRightYUpdated()
{   
    if (m_currentIndex > 0 && m_currentIndex < m_availableSizesList.size() 
        && ensureMilliMeter(m_optionBottomRightY, m_optionBottomRightY->value().toDouble()) != m_availableSizesList.at(m_currentIndex).height() ) {
        m_currentIndex = 0;
        Q_EMIT valueChanged(QPageSize::name(QPageSize::Custom));
    }
}

double PageSizeOption::ensureMilliMeter(BaseOption *option, double value) 
{
    // convert if necessary with current DPI if available
    if (option->valueUnit() == CoreOption::UnitPixel &&  m_optionResolution != nullptr) {
        double dpi = m_optionResolution->value().toDouble();
        if (dpi > 1) {
            return value / (dpi / 25.4);
        }
    }
    return value;
}

} // namespace KSane
