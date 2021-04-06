/* ============================================================
 *
 * This file is part of the KDE project
 *
 * Date        : 2007-09-13
 * Description : Sane interface for KDE
 *
 * SPDX-FileCopyrightText: 2007-2008 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2007-2008 Gilles Caulier <caulier dot gilles at gmail dot com>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * ============================================================ */

#include "ksanewidget_p.h"

#include <QImage>
#include <QScrollArea>
#include <QScrollBar>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QMetaMethod>
#include <QPageSize>

#include <ksane_debug.h>

#define SCALED_PREVIEW_MAX_SIDE 400

static constexpr int ActiveSelection = 100000;
static constexpr int PageSizeWiggleRoom = 2; // in mm

namespace KSaneIface
{

KSaneWidgetPrivate::KSaneWidgetPrivate(KSaneWidget *parent):
    q(parent)
{
    // Device independent UI variables
    m_optsTabWidget = nullptr;
    m_basicOptsTab  = nullptr;
    m_otherOptsTab  = nullptr;
    m_zInBtn        = nullptr;
    m_zOutBtn       = nullptr;
    m_zSelBtn       = nullptr;
    m_zFitBtn       = nullptr;
    m_clearSelBtn   = nullptr;
    m_prevBtn       = nullptr;
    m_scanBtn       = nullptr;
    m_cancelBtn     = nullptr;
    m_previewViewer = nullptr;
    m_autoSelect    = true;
    m_selIndex      = ActiveSelection;
    m_warmingUp     = nullptr;
    m_progressBar   = nullptr;

    // scanning variables
    m_isPreview     = false;

    m_saneHandle    = nullptr;
    m_previewThread = nullptr;
    m_scanThread    = nullptr;

    m_splitGamChB   = nullptr;
    m_commonGamma   = nullptr;
    m_previewDPI    = 0;

    m_previewWidth  = 0;
    m_previewHeight = 0;

    m_sizeCodes = {
        QPageSize::A4,
        QPageSize::A5,
        QPageSize::A6,
        QPageSize::Letter,
        QPageSize::Legal,
        QPageSize::Tabloid,
        QPageSize::A3,
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

    clearDeviceOptions();

    m_findDevThread = FindSaneDevicesThread::getInstance();
    connect(m_findDevThread, &FindSaneDevicesThread::finished, this, &KSaneWidgetPrivate::devListUpdated);
    connect(m_findDevThread, &FindSaneDevicesThread::finished, this, &KSaneWidgetPrivate::signalDevListUpdate);

    m_auth = KSaneAuth::getInstance();
    m_optionPollTmr.setInterval(100);
    connect(&m_optionPollTmr, &QTimer::timeout, this, &KSaneWidgetPrivate::pollPollOptions);
}

void KSaneWidgetPrivate::clearDeviceOptions()
{
    m_optSource     = nullptr;
    m_colorOpts     = nullptr;
    m_optNegative   = nullptr;
    m_optFilmType   = nullptr;
    m_optMode       = nullptr;
    m_optDepth      = nullptr;
    m_optRes        = nullptr;
    m_optResX       = nullptr;
    m_optResY       = nullptr;
    m_optTlX        = nullptr;
    m_optTlY        = nullptr;
    m_optBrX        = nullptr;
    m_optBrY        = nullptr;
    m_optGamR       = nullptr;
    m_optGamG       = nullptr;
    m_optGamB       = nullptr;
    m_optPreview    = nullptr;
    m_optWaitForBtn = nullptr;
    m_scanOngoing   = false;
    m_closeDevicePending = false;

    // delete all the options in the list.
    while (!m_optList.isEmpty()) {
        delete m_optList.takeFirst();
    }
    m_pollList.clear();
    m_optWithWidget.clear();
    m_optionPollTmr.stop();

    // remove the remaining layouts/widgets and read thread
    delete m_basicOptsTab;
    m_basicOptsTab = nullptr;

    delete m_otherOptsTab;
    m_otherOptsTab = nullptr;

    delete m_previewThread;
    m_previewThread = nullptr;

    delete m_scanThread;
    m_scanThread = nullptr;

    m_devName.clear();
    m_model.clear();
    m_vendor.clear();
    Q_EMIT q->openedDeviceInfoUpdated(m_devName, m_vendor, m_model);
}

void KSaneWidgetPrivate::devListUpdated()
{
    if (m_vendor.isEmpty()) {
        const QList<KSaneWidget::DeviceInfo> deviceList = m_findDevThread->devicesList();
        for (const auto &device : deviceList) {
            if (device.name == m_devName) {
                m_vendor    = device.vendor;
                m_model     = device.model;
                Q_EMIT q->openedDeviceInfoUpdated(m_devName, m_vendor, m_model);
                break;
            }
        }
    }
}

void KSaneWidgetPrivate::signalDevListUpdate()
{
    Q_EMIT q->availableDevices(m_findDevThread->devicesList());
}

KSaneWidget::ImageFormat KSaneWidgetPrivate::getImgFormat(SANE_Parameters &params)
{
    switch (params.format) {
    case SANE_FRAME_GRAY:
        switch (params.depth) {
        case 1:
            return KSaneWidget::FormatBlackWhite;
        case 8:
            return KSaneWidget::FormatGrayScale8;
        case 16:
            return KSaneWidget::FormatGrayScale16;
        default:
            return KSaneWidget::FormatNone;
        }
    case SANE_FRAME_RGB:
    case SANE_FRAME_RED:
    case SANE_FRAME_GREEN:
    case SANE_FRAME_BLUE:
        switch (params.depth) {
        case 8:
            return KSaneWidget::FormatRGB_8_C;
        case 16:
            return KSaneWidget::FormatRGB_16_C;
        default:
            return KSaneWidget::FormatNone;
        }
    }
    return KSaneWidget::FormatNone;
}

int KSaneWidgetPrivate::getBytesPerLines(SANE_Parameters &params)
{
    switch (getImgFormat(params)) {
    case KSaneWidget::FormatBlackWhite:
    case KSaneWidget::FormatGrayScale8:
    case KSaneWidget::FormatGrayScale16:
        return params.bytes_per_line;

    case KSaneWidget::FormatRGB_8_C:
        return params.pixels_per_line * 3;

    case KSaneWidget::FormatRGB_16_C:
        return params.pixels_per_line * 6;

    case KSaneWidget::FormatNone:
    case KSaneWidget::FormatBMP: // to remove warning (BMP is omly valid in the twain wrapper)
        return 0;
    }
    return 0;
}

float KSaneWidgetPrivate::ratioToScanAreaX(float ratio)
{
    if (!m_optBrX) {
        return 0.0;
    }
    float max = m_optBrX->getMaxValue().toFloat();

    return max * ratio;
}

float KSaneWidgetPrivate::ratioToScanAreaY(float ratio)
{
    if (!m_optBrY) {
        return 0.0;
    }
    float max = m_optBrY->getMaxValue().toFloat();

    return max * ratio;
}

float KSaneWidgetPrivate::scanAreaToRatioX(float scanArea)
{
    if (!m_optBrX) {
        return 0.0;
    }
    float max = m_optBrX->getMaxValue().toFloat();

    if (scanArea > max) {
        return 1.0;
    }

    if (max < 0.0001) {
        return 0.0;
    }

    return scanArea / max;
}

float KSaneWidgetPrivate::scanAreaToRatioY(float scanArea)
{
    if (!m_optBrY) {
        return 0.0;
    }
    float max = m_optBrY->getMaxValue().toFloat();

    if (scanArea > max) {
        return 1.0;
    }

    if (max < 0.0001) {
        return 0.0;
    }

    return scanArea / max;
}

static float mmToDispUnit(float mm) {
    static QLocale locale;

    if (locale.measurementSystem() == QLocale::MetricSystem) {
        return mm;
    }
    // else
    return mm / 25.4;
}

float KSaneWidgetPrivate::ratioToDispUnitX(float ratio)
{
    if (!m_optBrX) {
        return 0.0;
    }

    float result = ratioToScanAreaX(ratio);

    if (m_optBrX->getUnit() == KSaneOption::UnitMilliMeter) {
        return mmToDispUnit(result);
    }
    else if (m_optBrX->getUnit() == KSaneOption::UnitPixel && m_optRes) {
        // get current DPI
        float dpi = m_optRes->getValue().toFloat();
        if (dpi > 1) {
            result = result / (dpi / 25.4);
            return mmToDispUnit(result);
        }
    }
    qCDebug(KSANE_LOG) << "Failed to convert scan area to mm";
    return ratio;
}

float KSaneWidgetPrivate::ratioToDispUnitY(float ratio)
{
    if (!m_optBrY) {
        return 0.0;
    }

    float result = ratioToScanAreaY(ratio);

    if (m_optBrY->getUnit() == KSaneOption::UnitMilliMeter) {
        return mmToDispUnit(result);
    }
    else if (m_optBrY->getUnit() == KSaneOption::UnitPixel && m_optRes) {
        // get current DPI
        float dpi = m_optRes->getValue().toFloat();
        if (dpi > 1) {
            result = result / (dpi / 25.4);
            return mmToDispUnit(result);
        }
    }
    qCDebug(KSANE_LOG) << "Failed to convert scan area to display unit";
    return ratio;
}

float KSaneWidgetPrivate::dispUnitToRatioX(float value)
{
    float valueMax = ratioToDispUnitX(1);
    if (valueMax < 1) {
        return 0.0;
    }
    return value / valueMax;
}

float KSaneWidgetPrivate::dispUnitToRatioY(float value)
{
    float valueMax = ratioToDispUnitY(1);
    if (valueMax < 1) {
        return 0.0;
    }
    return value / valueMax;
}

KSaneOption *KSaneWidgetPrivate::getOption(const QString &name)
{
    int i;
    for (i = 0; i < m_optList.size(); ++i) {
        KSaneOption * option = m_optList.at(i);
        if (option->name() == name) {
            return option;
        }
    }
    return nullptr;
}

KSaneOptionWidget *KSaneWidgetPrivate::createOptionWidget(QWidget *parent, KSaneOption *option)
{
    KSaneOptionWidget *widget = nullptr;
    switch (option->type()) {
        case KSaneOption::TypeBool:
            widget = new LabeledCheckbox(parent, option);
            break;
        case KSaneOption::TypeInteger:
            widget = new LabeledSlider(parent, option);
            break;
        case KSaneOption::TypeDouble:
            widget = new LabeledFSlider(parent, option);
            break;
        case KSaneOption::TypeValueList:
            widget = new LabeledCombo(parent, option);
            break;
        case KSaneOption::TypeString:
            widget = new LabeledEntry(parent, option);
            break;
        case KSaneOption::TypeGamma:
            widget = new LabeledGamma(parent, option);
            break;
        case KSaneOption::TypeAction:
            widget = new KSaneButton(parent, option);
            break;
        default:
            widget = new KSaneOptionWidget(parent, option);
            break;
    }
    m_optWithWidget.insert(option->name());
    return widget;
}

void KSaneWidgetPrivate::createOptInterface()
{
    m_basicOptsTab = new QWidget;
    m_basicScrollA->setWidget(m_basicOptsTab);

    QVBoxLayout *basic_layout = new QVBoxLayout(m_basicOptsTab);
    KSaneOption *option = getOption(QStringLiteral(SANE_NAME_SCAN_SOURCE));
    // Scan Source
    if (option != nullptr) {
        m_optSource = option;
        KSaneOptionWidget *source = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(source);
        connect(m_optSource, &KSaneOption::valueChanged, this, &KSaneWidgetPrivate::checkInvert, Qt::QueuedConnection);
        connect(m_optSource, &KSaneOption::valueChanged, this, [this]() {
            m_previewViewer->setMultiselectionEnabled(!scanSourceADF());
        });
    }

    // film-type (note: No translation)
    if ((option = getOption(QStringLiteral("film-type"))) != nullptr) {
        m_optFilmType = option;
        KSaneOptionWidget *film = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(film);
        connect(m_optFilmType, &KSaneOption::valueChanged, this, &KSaneWidgetPrivate::checkInvert, Qt::QueuedConnection);
    } else if ((option = getOption(QStringLiteral(SANE_NAME_NEGATIVE))) != nullptr) {
        m_optNegative = option;
        KSaneOptionWidget *negative = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(negative);
    }
    // Scan mode
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_MODE))) != nullptr) {
        m_optMode = option;
        KSaneOptionWidget *mode = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(mode);
    }
    // Bitdepth
    if ((option = getOption(QStringLiteral(SANE_NAME_BIT_DEPTH))) != nullptr) {
        m_optDepth = option;
        KSaneOptionWidget *bitDepth = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(bitDepth);
    }
    // Threshold
    if ((option = getOption(QStringLiteral(SANE_NAME_THRESHOLD))) != nullptr) {
        KSaneOptionWidget *threshold = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(threshold);
    }
    // Resolution
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_RESOLUTION))) != nullptr) {
        m_optRes = option;
        KSaneOptionWidget *resolution = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(resolution);
    }
    // These two next resolution options are a bit tricky.
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_X_RESOLUTION))) != nullptr) {
        m_optResX = option;
        if (!m_optRes) {
            m_optRes = m_optResX;
        }
        KSaneOptionWidget *optResX = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(optResX);
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_Y_RESOLUTION))) != nullptr) {
        m_optResY = option;
        KSaneOptionWidget *optResY = createOptionWidget(m_basicOptsTab, option);
        basic_layout->addWidget(optResY);
    }

    // save a pointer to the preview option if possible
    if ((option = getOption(QStringLiteral(SANE_NAME_PREVIEW))) != nullptr) {
        m_optPreview = option;
    }

    // save a pointer to the "wait-for-button" option if possible (Note: No translation)
    if ((option = getOption(QStringLiteral("wait-for-button"))) != nullptr) {
        m_optWaitForBtn = option;
    }

    // scan area (Do not add the widgets)
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_TL_X))) != nullptr) {
        m_optTlX = option;
        connect(option, &KSaneOption::valueChanged, this, &KSaneWidgetPrivate::setTLX);
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_TL_Y))) != nullptr) {
        m_optTlY = option;
        connect(option, &KSaneOption::valueChanged, this, &KSaneWidgetPrivate::setTLY);
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_BR_X))) != nullptr) {
        m_optBrX = option;
        connect(option, &KSaneOption::valueChanged, this, &KSaneWidgetPrivate::setBRX);
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_BR_Y))) != nullptr) {
        m_optBrY = option;
        connect(option, &KSaneOption::valueChanged, this, &KSaneWidgetPrivate::setBRY);
    }

    // Color Options Frame
    m_colorOpts = new QWidget(m_basicOptsTab);
    basic_layout->addWidget(m_colorOpts);
    QVBoxLayout *color_lay = new QVBoxLayout(m_colorOpts);
    color_lay->setContentsMargins(0, 0, 0, 0);

    // Add Color correction to the color "frame"
    if ((option = getOption(QStringLiteral(SANE_NAME_BRIGHTNESS))) != nullptr) {
        KSaneOptionWidget *brightness = createOptionWidget(m_basicOptsTab, option);
        color_lay->addWidget(brightness);
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_CONTRAST))) != nullptr) {
        KSaneOptionWidget *contrast = createOptionWidget(m_basicOptsTab, option);
        color_lay->addWidget(contrast);
    }

    // Add gamma tables to the color "frame"
    QWidget *gamma_frm = new QWidget(m_colorOpts);
    color_lay->addWidget(gamma_frm);
    QVBoxLayout *gam_frm_l = new QVBoxLayout(gamma_frm);
    gam_frm_l->setContentsMargins(0, 0, 0, 0);
    LabeledGamma *gammaR = nullptr;
    LabeledGamma *gammaG = nullptr;
    LabeledGamma *gammaB = nullptr;
    if ((option = getOption(QStringLiteral(SANE_NAME_GAMMA_VECTOR_R))) != nullptr) {
        m_optGamR = option;
        gammaR = new LabeledGamma(gamma_frm, option);
        gam_frm_l->addWidget(gammaR);
        m_optWithWidget.insert(option->name());
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_GAMMA_VECTOR_G))) != nullptr) {
        m_optGamG = option;
        gammaG = new LabeledGamma(gamma_frm, option);
        gam_frm_l->addWidget(gammaG);
        m_optWithWidget.insert(option->name());
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_GAMMA_VECTOR_B))) != nullptr) {
        m_optGamB = option;
        gammaB = new LabeledGamma(gamma_frm, option);
        gam_frm_l->addWidget(gammaB);
        m_optWithWidget.insert(option->name());
    }

    if ((m_optGamR != nullptr) && (m_optGamG != nullptr) && (m_optGamB != nullptr) 
        && (gammaR != nullptr) && (gammaG != nullptr) && (gammaB != nullptr)  ) {
        
        m_commonGamma = new LabeledGamma(m_colorOpts, i18n(SANE_TITLE_GAMMA_VECTOR), gammaR->maxValue());

        color_lay->addWidget(m_commonGamma);

        m_commonGamma->setToolTip(i18n(SANE_DESC_GAMMA_VECTOR));

        connect(m_commonGamma, &LabeledGamma::valuesChanged, gammaR, &LabeledGamma::setValues);
        connect(m_commonGamma, &LabeledGamma::valuesChanged, gammaG, &LabeledGamma::setValues);
        connect(m_commonGamma, &LabeledGamma::valuesChanged, gammaB, &LabeledGamma::setValues);

        m_splitGamChB = new LabeledCheckbox(m_colorOpts, i18n("Separate color intensity tables"));
        color_lay->addWidget(m_splitGamChB);

        connect(m_splitGamChB, &LabeledCheckbox::toggled, gamma_frm, &QWidget::setVisible);
        connect(m_splitGamChB, &LabeledCheckbox::toggled, m_commonGamma, &LabeledGamma::setHidden);

        gamma_frm->hide();
    }

    if ((option = getOption(QStringLiteral(SANE_NAME_BLACK_LEVEL))) != nullptr) {
        KSaneOptionWidget *blackLevel = createOptionWidget(m_colorOpts, option);
        color_lay->addWidget(blackLevel);
    }
    if ((option = getOption(QStringLiteral(SANE_NAME_WHITE_LEVEL))) != nullptr) {
        KSaneOptionWidget *blackLevel = createOptionWidget(m_colorOpts, option);
        color_lay->addWidget(blackLevel);
    }
    
    if ((option = getOption(InvertColorsOptionName)) != nullptr) {
        m_optInvert = option;
        KSaneOptionWidget *invertColor = createOptionWidget(m_colorOpts, option);
        color_lay->addWidget(invertColor);
        connect(m_optInvert, &KSaneOption::valueChanged, this, &KSaneWidgetPrivate::invertPreview);
    }

    // Add our own size options
    m_scanareaPapersize = new LabeledCombo(m_basicOptsTab, i18n("Scan Area Size"));
    connect(m_scanareaPapersize, &LabeledCombo::activated, this, &KSaneWidgetPrivate::setPageSize);
    basic_layout->addWidget(m_scanareaPapersize);

    static QLocale locale;
    QString unitSuffix = locale.measurementSystem() == QLocale::MetricSystem ? i18n(" mm") : i18n(" inch");

    m_scanareaWidth = new LabeledFSlider(m_basicOptsTab, i18n("Width"), 0.0f, 500.0f, 0.1f);
    m_scanareaWidth->setSuffix(unitSuffix);
    connect(m_scanareaWidth, &LabeledFSlider::valueChanged, this, &KSaneWidgetPrivate::updateScanSelection);
    basic_layout->addWidget(m_scanareaWidth);

    m_scanareaHeight = new LabeledFSlider(m_basicOptsTab, i18n("Height"), 0.0f, 500.0f, 0.1f);
    m_scanareaHeight->setSuffix(unitSuffix);
    connect(m_scanareaHeight, &LabeledFSlider::valueChanged, this, &KSaneWidgetPrivate::updateScanSelection);
    basic_layout->addWidget(m_scanareaHeight);

    m_scanareaX = new LabeledFSlider(m_basicOptsTab, i18n("X Offset"), 0.0f, 500.0f, 0.1f);
    m_scanareaX->setSuffix(unitSuffix);
    connect(m_scanareaX, &LabeledFSlider::valueChanged, this, &KSaneWidgetPrivate::updateScanSelection);
    basic_layout->addWidget(m_scanareaX);

    m_scanareaY = new LabeledFSlider(m_basicOptsTab, i18n("Y Offset"), 0.0f, 500.0f, 0.1f);
    m_scanareaY->setSuffix(unitSuffix);
    connect(m_scanareaY, &LabeledFSlider::valueChanged, this, &KSaneWidgetPrivate::updateScanSelection);
    basic_layout->addWidget(m_scanareaY);

    // add a stretch to the end to keep the parameters at the top
    basic_layout->addStretch();

    // Remaining (un known) options go to the "Other Options"
    m_otherOptsTab = new QWidget;
    m_otherScrollA->setWidget(m_otherOptsTab);

    QVBoxLayout *other_layout = new QVBoxLayout(m_otherOptsTab);

    // add the remaining parameters
    for (int i = 0; i < m_optList.size(); ++i) {
        KSaneOption *option = m_optList.at(i);
        if (m_optWithWidget.find(option->name()) != m_optWithWidget.end()) {
            continue;
        }
        if ((option->name() != QStringLiteral(SANE_NAME_SCAN_TL_X)) &&
            (option->name() != QStringLiteral(SANE_NAME_SCAN_TL_Y)) &&
            (option->name() != QStringLiteral(SANE_NAME_SCAN_BR_X)) &&
            (option->name() != QStringLiteral(SANE_NAME_SCAN_BR_Y)) &&
            (option->name() != QStringLiteral(SANE_NAME_PREVIEW)) &&
            (option->type() != KSaneOption::TypeDetectFail)) {
            KSaneOptionWidget *widget = createOptionWidget(m_otherOptsTab, option);
            if (widget != nullptr) {
                other_layout->addWidget(widget);
            }
        }
    }

    // add a stretch to the end to keep the parameters at the top
    other_layout->addStretch();

    // calculate label widths
    int labelWidth = 0;
    KSaneOptionWidget *tmpOption;
    // Basic Options
    for (int i = 0; i < basic_layout->count(); ++i) {
        if (basic_layout->itemAt(i) && basic_layout->itemAt(i)->widget()) {
            tmpOption = qobject_cast<KSaneOptionWidget *>(basic_layout->itemAt(i)->widget());
            if (tmpOption) {
                labelWidth = qMax(labelWidth, tmpOption->labelWidthHint());
            }
        }
    }
    // Color Options
    for (int i = 0; i < color_lay->count(); ++i) {
        if (color_lay->itemAt(i) && color_lay->itemAt(i)->widget()) {
            tmpOption = qobject_cast<KSaneOptionWidget *>(color_lay->itemAt(i)->widget());
            if (tmpOption) {
                labelWidth = qMax(labelWidth, tmpOption->labelWidthHint());
            }
        }
    }
    // Set label widths
    for (int i = 0; i < basic_layout->count(); ++i) {
        if (basic_layout->itemAt(i) && basic_layout->itemAt(i)->widget()) {
            tmpOption = qobject_cast<KSaneOptionWidget *>(basic_layout->itemAt(i)->widget());
            if (tmpOption) {
                tmpOption->setLabelWidth(labelWidth);
            }
        }
    }
    for (int i = 0; i < color_lay->count(); ++i) {
        if (color_lay->itemAt(i) && color_lay->itemAt(i)->widget()) {
            tmpOption = qobject_cast<KSaneOptionWidget *>(color_lay->itemAt(i)->widget());
            if (tmpOption) {
                tmpOption->setLabelWidth(labelWidth);
            }
        }
    }
    // Other Options
    labelWidth = 0;
    for (int i = 0; i < other_layout->count(); ++i) {
        if (other_layout->itemAt(i) && other_layout->itemAt(i)->widget()) {
            tmpOption = qobject_cast<KSaneOptionWidget *>(other_layout->itemAt(i)->widget());
            if (tmpOption) {
                labelWidth = qMax(labelWidth, tmpOption->labelWidthHint());
            }
        }
    }
    for (int i = 0; i < other_layout->count(); ++i) {
        if (other_layout->itemAt(i) && other_layout->itemAt(i)->widget()) {
            tmpOption = qobject_cast<KSaneOptionWidget *>(other_layout->itemAt(i)->widget());
            if (tmpOption) {
                tmpOption->setLabelWidth(labelWidth);
            }
        }
    }

    // ensure that we do not get a scrollbar at the bottom of the option of the options
    int min_width = m_basicOptsTab->sizeHint().width();
    if (min_width < m_otherOptsTab->sizeHint().width()) {
        min_width = m_otherOptsTab->sizeHint().width();
    }

    m_optsTabWidget->setMinimumWidth(min_width + m_basicScrollA->verticalScrollBar()->sizeHint().width() + 5);
}

void KSaneWidgetPrivate::setDefaultValues()
{
    KSaneOption *option;

    // Try to get Color mode by default
    if ((option = getOption(QStringLiteral(SANE_NAME_SCAN_MODE))) != nullptr) {
        option->setValue(i18n(SANE_VALUE_SCAN_MODE_COLOR));
    }

    // Try to set 8 bit color
    if ((option = getOption(QStringLiteral(SANE_NAME_BIT_DEPTH))) != nullptr) {
        option->setValue(8);
    }

    // Try to set Scan resolution to 600 DPI
    if (m_optRes != nullptr) {
        m_optRes->setValue(600);
    }
}

void KSaneWidgetPrivate::scheduleValuesReload()
{
    m_readValsTmr.start(5);
}

void KSaneWidgetPrivate::reloadOptions()
{
    int i;

    for (i = 0; i < m_optList.size(); ++i) {
        m_optList.at(i)->readOption();
        // Also read the values
        m_optList.at(i)->readValue();
    }
    // Gamma table special case
    if (m_optGamR && m_optGamG && m_optGamB) {
        m_commonGamma->setHidden(m_optGamR->state() == KSaneOption::StateHidden);
        m_splitGamChB->setHidden(m_optGamR->state() == KSaneOption::StateHidden);
    }

    // estimate the preview size and create an empty image
    // this is done so that you can select scan area without
    // having to scan a preview.
    updatePreviewSize();

    // ensure that we do not get a scrollbar at the bottom of the option of the options
    int min_width = m_basicOptsTab->sizeHint().width();
    if (min_width < m_otherOptsTab->sizeHint().width()) {
        min_width = m_otherOptsTab->sizeHint().width();
    }

    m_optsTabWidget->setMinimumWidth(min_width + m_basicScrollA->verticalScrollBar()->sizeHint().width() + 5);

    m_previewViewer->zoom2Fit();
}

void KSaneWidgetPrivate::reloadValues()
{
    for (int i = 0; i < m_optList.size(); ++i) {
        m_optList.at(i)->readValue();
    }

}

void KSaneWidgetPrivate::handleSelection(float tl_x, float tl_y, float br_x, float br_y)
{
    if ((m_optTlX == nullptr) || (m_optTlY == nullptr) || (m_optBrX == nullptr) || (m_optBrY == nullptr)) {
        // clear the selection since we can not set one
        m_previewViewer->setTLX(0);
        m_previewViewer->setTLY(0);
        m_previewViewer->setBRX(0);
        m_previewViewer->setBRY(0);
        return;
    }

    if ((m_previewImg.width() == 0) || (m_previewImg.height() == 0)) {
        m_scanareaX->setValue(0);
        m_scanareaY->setValue(0);

        m_scanareaWidth->setValue(ratioToDispUnitX(1));
        m_scanareaHeight->setValue(ratioToDispUnitY(1));
       return;
    }

    if (br_x < 0.0001) {
        m_scanareaWidth->setValue(ratioToDispUnitX(1));
        m_scanareaHeight->setValue(ratioToDispUnitY(1));
    }
    else {
        m_scanareaWidth->setValue(ratioToDispUnitX(br_x - tl_x));
        m_scanareaHeight->setValue(ratioToDispUnitY(br_y - tl_y));
    }
    m_scanareaX->setValue(ratioToDispUnitX(tl_x));
    m_scanareaY->setValue(ratioToDispUnitY(tl_y));

    m_optTlX->setValue(ratioToScanAreaX(tl_x));
    m_optTlY->setValue(ratioToScanAreaY(tl_y));
    m_optBrX->setValue(ratioToScanAreaX(br_x));
    m_optBrY->setValue(ratioToScanAreaY(br_y));
}

void KSaneWidgetPrivate::setTLX(const QVariant &x)
{
    bool ok;
    float ftlx = x.toFloat(&ok);
    // ignore this when conversion not possible and during an active scan
    if (!ok || m_previewThread->isRunning() || m_scanThread->isRunning() || m_scanOngoing) {
        return;
    }

    float ratio = scanAreaToRatioX(ftlx);
    m_previewViewer->setTLX(ratio);
    m_scanareaX->setValue(ratioToDispUnitX(ratio));
}

void KSaneWidgetPrivate::setTLY(const QVariant &y)
{
    bool ok;
    float ftly = y.toFloat(&ok);
    // ignore this when conversion not possible and during an active scan
    if (!ok || m_previewThread->isRunning() || m_scanThread->isRunning() || m_scanOngoing) {
        return;
    }

    float ratio = scanAreaToRatioY(ftly);
    m_previewViewer->setTLY(ratio);
    m_scanareaY->setValue(ratioToDispUnitY(ratio));
}

void KSaneWidgetPrivate::setBRX(const QVariant &x)
{
    bool ok;
    float fbrx = x.toFloat(&ok);
    // ignore this when conversion not possible and during an active scan
    if (!ok || m_previewThread->isRunning() || m_scanThread->isRunning() || m_scanOngoing) {
        return;
    }

    float ratio = scanAreaToRatioX(fbrx);
    m_previewViewer->setBRX(ratio);

    if (!m_optTlX) {
        return;
    }
    
    QVariant tlx = m_optTlX->getValue();
    if (!tlx.isNull()) {
        float tlxRatio = scanAreaToRatioX(tlx.toFloat());
        m_scanareaWidth->setValue(ratioToDispUnitX(ratio) - ratioToDispUnitX(tlxRatio));
    }
}

void KSaneWidgetPrivate::setBRY(const QVariant &y)
{
    bool ok;
    float fbry = y.toFloat(&ok);
    // ignore this when conversion not possible and during an active scan
    if (!ok || m_previewThread->isRunning() || m_scanThread->isRunning() || m_scanOngoing) {
        return;
    }

    float ratio = scanAreaToRatioY(fbry);
    m_previewViewer->setBRY(ratio);
    
    if (!m_optTlY) {
        return;
    }
    QVariant tly = m_optTlY->getValue();
    if (!tly.isNull()) {
        float tlyRatio = scanAreaToRatioY(tly.toFloat());
        m_scanareaHeight->setValue(ratioToDispUnitY(ratio) - ratioToDispUnitY(tlyRatio));
    }
}

void KSaneWidgetPrivate::updatePreviewSize()
{
    float max_x = 0;
    float max_y = 0;
    float ratio;
    int x, y;

    // check if an update is necessary
    if (m_optBrX != nullptr) {
        max_x = m_optBrX->getMaxValue().toFloat();
    }
    if (m_optBrY != nullptr) {
        max_y = m_optBrY->getMaxValue().toFloat();
    }
    if ((max_x == m_previewWidth) && (max_y == m_previewHeight)) {
        //qCDebug(KSANE_LOG) << "no preview size change";
        return;
    }

    // The preview size has changed
    m_previewWidth  = max_x;
    m_previewHeight = max_y;

    // set the scan area to the whole area
    m_previewViewer->clearSelections();
    if (m_optTlX != nullptr) {
        m_optTlX->setValue(0);
    }
    if (m_optTlY != nullptr) {
        m_optTlY->setValue(0);
    }

    if (m_optBrX != nullptr) {
        m_optBrX->setValue(max_x);
    }
    if (m_optBrY != nullptr) {
        m_optBrY->setValue(max_y);
    }

    // Avoid crash if max_y or max_x == 0
    if (max_x < 0.0001 || max_y < 0.0001) {
        qCWarning(KSANE_LOG) << "Risk for division by 0" << max_x << max_y;
        return;
    }

    // create a "scaled" image of the preview
    ratio = max_x / max_y;
    if (ratio < 1) {
        x = SCALED_PREVIEW_MAX_SIDE;
        y = (int)(SCALED_PREVIEW_MAX_SIDE / ratio);
    } else {
        y = SCALED_PREVIEW_MAX_SIDE;
        x = (int)(SCALED_PREVIEW_MAX_SIDE / ratio);
    }

    const qreal dpr = q->devicePixelRatioF();
    m_previewImg = QImage(QSize(x, y) * dpr, QImage::Format_RGB32);
    m_previewImg.setDevicePixelRatio(dpr);
    m_previewImg.fill(0xFFFFFFFF);

    // set the new image
    m_previewViewer->setQImage(&m_previewImg);

    // update the scan-area-options
    m_scanareaWidth->setRange(0.1, ratioToDispUnitX(1));
    m_scanareaWidth->setValue(ratioToDispUnitX(1));

    m_scanareaHeight->setRange(0.1, ratioToDispUnitY(1));
    m_scanareaHeight->setValue(ratioToDispUnitY(1));

    m_scanareaX->setRange(0.0, ratioToDispUnitX(1));
    m_scanareaY->setRange(0.0, ratioToDispUnitY(1));

    setPossibleScanSizes();
}

void KSaneWidgetPrivate::startPreviewScan()
{
    if (m_scanOngoing) {
        return;
    }
    m_scanOngoing = true;

    SANE_Status status;
    float max_x, max_y;
    float dpi;

    // store the current settings of parameters to be changed
    if (m_optDepth != nullptr) {
        m_optDepth->storeCurrentData();
    }
    if (m_optRes != nullptr) {
        m_optRes->storeCurrentData();
    }
    if (m_optResX != nullptr) {
        m_optResX->storeCurrentData();
    }
    if (m_optResY != nullptr) {
        m_optResY->storeCurrentData();
    }
    if (m_optPreview != nullptr) {
        m_optPreview->storeCurrentData();
    }

    // check if we can modify the selection
    if ((m_optTlX != nullptr) && (m_optTlY != nullptr) &&
            (m_optBrX != nullptr) && (m_optBrY != nullptr)) {
        // get maximums
        max_x = m_optBrX->getMaxValue().toFloat();
        max_y = m_optBrY->getMaxValue().toFloat();
        // select the whole area
        m_optTlX->setValue(0);
        m_optTlY->setValue(0);
        m_optBrX->setValue(max_x);
        m_optBrY->setValue(max_y);

    } else {
        // no use to try auto selections if you can not use them
        m_autoSelect = false;
    }

    if (m_optRes != nullptr) {
        if (m_previewDPI >= 25.0) {
            m_optRes->setValue(m_previewDPI);
            if ((m_optResY != nullptr) && (m_optRes->name() == QStringLiteral(SANE_NAME_SCAN_X_RESOLUTION))) {
                m_optResY->setValue(m_previewDPI);
            }
        } else {
            // set the resolution to getMinValue and increase if necessary
            SANE_Parameters params;
            dpi = m_optRes->getMinValue().toFloat();
            do {
                m_optRes->setValue(dpi);
                if ((m_optResY != nullptr) && (m_optRes->name() == QStringLiteral(SANE_NAME_SCAN_X_RESOLUTION))) {
                    m_optResY->setValue(dpi);
                }
                //check what image size we would get in a scan
                status = sane_get_parameters(m_saneHandle, &params);
                if (status != SANE_STATUS_GOOD) {
                    qCDebug(KSANE_LOG) << "sane_get_parameters=" << sane_strstatus(status);
                    previewScanDone();
                    return;
                }

                if (dpi > 600) {
                    break;
                }

                // Increase the dpi value
                dpi += 25.0;
            } while ((params.pixels_per_line < 300) || ((params.lines > 0) && (params.lines < 300)));

            if (params.pixels_per_line == 0) {
                // This is a security measure for broken backends
                dpi = m_optRes->getMinValue().toFloat();
                m_optRes->setValue(dpi);
                qCDebug(KSANE_LOG) << "Setting minimum DPI value for a broken back-end";
            }
        }
    }

    // set preview option to true if possible
    if (m_optPreview != nullptr) {
        m_optPreview->setValue(SANE_TRUE);
    }

    // execute valReload if there is a pending value reload
    while (m_readValsTmr.isActive()) {
        m_readValsTmr.stop();
        reloadValues();
    }

    // clear the preview
    m_previewViewer->clearHighlight();
    m_previewViewer->clearSelections();
    m_previewImg.fill(0xFFFFFFFF);
    updatePreviewSize();

    setBusy(true);

    m_progressBar->setValue(0);
    m_isPreview = true;
    m_previewThread->setPreviewInverted(m_optInvert->getValue().toBool());
    m_previewThread->start();
    m_updProgressTmr.start();
}

void KSaneWidgetPrivate::previewScanDone()
{
    // even if the scan is finished successfully we need to call sane_cancel()
    sane_cancel(m_saneHandle);

    if (m_closeDevicePending) {
        setBusy(false);
        sane_close(m_saneHandle);
        m_saneHandle = nullptr;
        clearDeviceOptions();
        Q_EMIT q->scanDone(KSaneWidget::NoError, QString());
        return;
    }

    // restore the original settings of the changed parameters
    if (m_optDepth != nullptr) {
        m_optDepth->restoreSavedData();
    }
    if (m_optRes != nullptr) {
        m_optRes->restoreSavedData();
    }
    if (m_optResX != nullptr) {
        m_optResX->restoreSavedData();
    }
    if (m_optResY != nullptr) {
        m_optResY->restoreSavedData();
    }
    if (m_optPreview != nullptr) {
        m_optPreview->restoreSavedData();
    }

    m_previewViewer->setQImage(&m_previewImg);
    m_previewViewer->zoom2Fit();

    if ((m_previewThread->saneStatus() != SANE_STATUS_GOOD) &&
            (m_previewThread->saneStatus() != SANE_STATUS_EOF)) {
        alertUser(KSaneWidget::ErrorGeneral, sane_i18n(sane_strstatus(m_previewThread->saneStatus())));
    } else if (m_autoSelect) {
        m_previewViewer->findSelections();
    }

    setBusy(false);
    m_scanOngoing = false;
    m_updProgressTmr.stop();

    Q_EMIT q->scanDone(KSaneWidget::NoError, QString());

    return;
}

void KSaneWidgetPrivate::startFinalScan()
{
    if (m_scanOngoing) {
        return;
    }
    m_scanOngoing = true;
    m_isPreview = false;
    m_cancelMultiScan = false;

    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    m_selIndex = 0;

    if ((m_optTlX != nullptr) && (m_optTlY != nullptr) && (m_optBrX != nullptr) && (m_optBrY != nullptr)) {
        // read the selection from the viewer
        m_previewViewer->selectionAt(m_selIndex, x1, y1, x2, y2);
        m_previewViewer->setHighlightArea(x1, y1, x2, y2);
        m_selIndex++;

        // now set the selection
        m_optTlX->setValue(ratioToScanAreaX(x1));
        m_optTlY->setValue(ratioToScanAreaY(y1));
        m_optBrX->setValue(ratioToScanAreaX(x2));
        m_optBrY->setValue(ratioToScanAreaY(y2));
    }

    // execute a pending value reload
    while (m_readValsTmr.isActive()) {
        m_readValsTmr.stop();
        reloadValues();
    }

    setBusy(true);
    m_updProgressTmr.start();
    m_scanThread->setImageInverted(m_optInvert->getValue().toBool());
    m_scanThread->start();
}

bool KSaneWidgetPrivate::scanSourceADF()
{
    if (!m_optSource) {
        return false;
    }

    QString source = m_optSource->getValue().toString();

    return source.contains(QStringLiteral("Automatic Document Feeder")) ||
    source.contains(QStringLiteral("ADF")) ||
    source.contains(QStringLiteral("Duplex"));
}

void KSaneWidgetPrivate::oneFinalScanDone()
{
    m_updProgressTmr.stop();
    updateProgress();

    if (m_closeDevicePending) {
        setBusy(false);
        sane_close(m_saneHandle);
        m_saneHandle = nullptr;
        clearDeviceOptions();
        return;
    }

    if (m_scanThread->frameStatus() == KSaneScanThread::READ_READY) {
        // scan finished OK
        SANE_Parameters params = m_scanThread->saneParameters();
        int lines = params.lines;
        if (lines == -1) {
            // this is probably a handscanner -> calculate the size from the read data
            int bytesPerLine = qMax(getBytesPerLines(params), 1); // ensure no div by 0
            lines = m_scanData.size() / bytesPerLine;
        }
        Q_EMIT q->imageReady(m_scanData,
                           params.pixels_per_line,
                           lines,
                           getBytesPerLines(params),
                           (int)getImgFormat(params));

        // now check if we should have automatic ADF batch scanning
        if (scanSourceADF() && !m_cancelMultiScan) {
            // in batch mode only one area can be scanned per page
            m_updProgressTmr.start();
            m_scanThread->start();
            m_cancelMultiScan = false;
            return;
        }

        // Check if we have a "wait for button" batch scanning
        if (m_optWaitForBtn) {
            qCDebug(KSANE_LOG) << m_optWaitForBtn->name();
            QString wait = m_optWaitForBtn->getValue().toString();

            qCDebug(KSANE_LOG) << "wait ==" << wait;
            if (wait == QStringLiteral("true")) {
                // in batch mode only one area can be scanned per page
                //qCDebug(KSANE_LOG) << "source == \"Automatic Document Feeder\"";
                m_updProgressTmr.start();
                m_scanThread->start();
                return;
            }
        }

        // not batch scan, call sane_cancel to be able to change parameters.
        sane_cancel(m_saneHandle);

        //qCDebug(KSANE_LOG) << "index=" << m_selIndex << "size=" << m_previewViewer->selListSize();
        // check if we have multiple selections.
        if (m_previewViewer->selListSize() > m_selIndex) {
            if ((m_optTlX != nullptr) && (m_optTlY != nullptr) && (m_optBrX != nullptr) && (m_optBrY != nullptr)) {
                float x1 = 0;
                float y1 = 0;
                float x2 = 0;
                float y2 = 0;

                // read the selection from the viewer
                m_previewViewer->selectionAt(m_selIndex, x1, y1, x2, y2);

                // set the highlight
                m_previewViewer->setHighlightArea(x1, y1, x2, y2);

                // now set the selection
                m_optTlX->setValue(ratioToScanAreaX(x1));
                m_optTlY->setValue(ratioToScanAreaY(y1));
                m_optBrX->setValue(ratioToScanAreaX(x2));
                m_optBrY->setValue(ratioToScanAreaY(y2));
                m_selIndex++;

                // execute a pending value reload
                while (m_readValsTmr.isActive()) {
                    m_readValsTmr.stop();
                    reloadValues();
                }

                if (!m_cancelMultiScan) {
                    m_updProgressTmr.start();
                    m_scanThread->start();
                    m_cancelMultiScan = false;
                    return;
                }
            }
        }
        Q_EMIT q->scanDone(KSaneWidget::NoError, QString());
    } else {
        switch (m_scanThread->saneStatus()) {
        case SANE_STATUS_GOOD:
        case SANE_STATUS_CANCELLED:
        case SANE_STATUS_EOF:
            break;
        case SANE_STATUS_NO_DOCS:
            Q_EMIT q->scanDone(KSaneWidget::Information, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            alertUser(KSaneWidget::Information, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            break;
        case SANE_STATUS_UNSUPPORTED:
        case SANE_STATUS_IO_ERROR:
        case SANE_STATUS_NO_MEM:
        case SANE_STATUS_INVAL:
        case SANE_STATUS_JAMMED:
        case SANE_STATUS_COVER_OPEN:
        case SANE_STATUS_DEVICE_BUSY:
        case SANE_STATUS_ACCESS_DENIED:
            Q_EMIT q->scanDone(KSaneWidget::ErrorGeneral, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            alertUser(KSaneWidget::ErrorGeneral, sane_i18n(sane_strstatus(m_scanThread->saneStatus())));
            break;
        }
    }

    sane_cancel(m_saneHandle);

    // clear the highlight
    m_previewViewer->setHighlightArea(0, 0, 1, 1);
    setBusy(false);
    m_scanOngoing = false;
}

void KSaneWidgetPrivate::setBusy(bool busy)
{
    if (busy) {
        m_warmingUp->show();
        m_activityFrame->hide();
        m_btnFrame->hide();
        m_optionPollTmr.stop();
        Q_EMIT q->scanProgress(0);
    } else {
        m_warmingUp->hide();
        m_activityFrame->hide();
        m_btnFrame->show();
        if (m_pollList.size() > 0) {
            m_optionPollTmr.start();
        }
        Q_EMIT q->scanProgress(100);
    }

    m_optsTabWidget->setDisabled(busy);
    m_previewViewer->setDisabled(busy);

    m_scanBtn->setFocus(Qt::OtherFocusReason);
}

void KSaneWidgetPrivate::checkInvert()
{
    if (!m_optSource) {
        return;
    }
    if (!m_optFilmType) {
        return;
    }
    if (m_scanOngoing) {
        return;
    }

    QString source = m_optSource->getValue().toString();
    QString filmtype = m_optFilmType->getValue().toString();

    if ((source.contains(i18nc("This is compared to the option string returned by sane",
                               "Transparency"), Qt::CaseInsensitive)) &&
            (filmtype.contains(i18nc("This is compared to the option string returned by sane",
                                     "Negative"), Qt::CaseInsensitive))) {
        m_optInvert->setValue(true);
    } else {
        m_optInvert->setValue(false);
    }
}

void KSaneWidgetPrivate::invertPreview()
{
    m_previewImg.invertPixels();
    m_previewViewer->updateImage();
}

void KSaneWidgetPrivate::updateProgress()
{
    int progress;
    if (m_isPreview) {
        progress = m_previewThread->scanProgress();
        if (m_previewThread->saneStartDone()) {
            if (!m_progressBar->isVisible() || m_previewThread->imageResized()) {
                m_warmingUp->hide();
                m_activityFrame->show();
                // the image size might have changed
                m_previewThread->imgMutex.lock();
                m_previewViewer->setQImage(&m_previewImg);
                m_previewViewer->zoom2Fit();
                m_previewThread->imgMutex.unlock();
            } else {
                m_previewThread->imgMutex.lock();
                m_previewViewer->updateImage();
                m_previewThread->imgMutex.unlock();
            }
        }
    } else {
        if (!m_progressBar->isVisible() && (m_scanThread->saneStartDone())) {
            m_warmingUp->hide();
            m_activityFrame->show();
        }
        progress = m_scanThread->scanProgress();
        m_previewViewer->setHighlightShown(progress);
    }

    m_progressBar->setValue(progress);
    Q_EMIT q->scanProgress(progress);
}

void KSaneWidgetPrivate::alertUser(int type, const QString &strStatus)
{
    if (!q->isSignalConnected(QMetaMethod::fromSignal(&KSaneWidget::userMessage))) {
        switch (type) {
        case KSaneWidget::ErrorGeneral:
            QMessageBox::critical(nullptr, i18nc("@title:window", "General Error"), strStatus);
            break;
        default:
            QMessageBox::information(nullptr, i18nc("@title:window", "Information"), strStatus);
            break;
        }
    } else {
        Q_EMIT q->userMessage(type, strStatus);
    }
}

void KSaneWidgetPrivate::pollPollOptions()
{
    for (int i = 1; i < m_pollList.size(); ++i) {
        m_pollList.at(i)->readValue();
    }
}

void KSaneWidgetPrivate::updateScanSelection()
{
    QVariant maxX;
    if (m_optBrX) {
        maxX = m_optBrX->getMaxValue();
    }

    QVariant maxY;
    if (m_optBrY) {
        maxY = m_optBrY->getMaxValue();
    }

    float x1 = m_scanareaX->value();
    float y1 = m_scanareaY->value();
    float w = m_scanareaWidth->value();
    float h = m_scanareaHeight->value();

    float x1Max = maxX.toFloat() - w;
    m_scanareaX->setRange(0.0, x1Max);
    if (x1 > x1Max) {
        m_scanareaX->setValue(x1Max);
    }

    float y1Max = maxY.toFloat() - h;
    m_scanareaY->setRange(0.0, y1Max);
    if (y1 > y1Max) {
        m_scanareaY->setValue(y1Max);
    }

    float wR = dispUnitToRatioX(w);
    float hR = dispUnitToRatioY(h);

    float x1R = dispUnitToRatioX(m_scanareaX->value());
    float y1R = dispUnitToRatioY(m_scanareaY->value());

    m_previewViewer->setSelection(x1R, y1R, x1R+wR, y1R+hR);

    // Update the page size combo, but not while updating or
    // if we already have custom page size.
    if (m_settingPageSize ||
        m_scanareaPapersize->currentIndex() == m_scanareaPapersize->count() - 1) {
        return;
    }
    QSizeF size = m_scanareaPapersize->currentData().toSizeF();
    float pageWidth = mmToDispUnit(size.width());
    float pageHeight = mmToDispUnit(size.height());
    if (qAbs(pageWidth - w) > (w * 0.001) ||
        qAbs(pageHeight - h) > (h * 0.001))
    {
        // The difference is bigger than 1% -> we have a custom size
        m_scanareaPapersize->blockSignals(true);
        m_scanareaPapersize->setCurrentIndex(0); // Custom is always first
        m_scanareaPapersize->blockSignals(false);
    }
}

void KSaneWidgetPrivate::setPossibleScanSizes()
{
    m_scanareaPapersize->clear();
    float widthInDispUnit = ratioToDispUnitX(1);
    float heightInDispUnit = ratioToDispUnitY(1);

    // Add the custom size first
    QSizeF customSize(widthInDispUnit, heightInDispUnit);
    m_scanareaPapersize->addItem(i18n("Custom"), customSize);

    // Add portrait page sizes
    for (int sizeCode: qAsConst(m_sizeCodes)) {
        QSizeF size = QPageSize::size((QPageSize::PageSizeId)sizeCode, QPageSize::Millimeter);
        if (mmToDispUnit(size.width() - PageSizeWiggleRoom) > widthInDispUnit) {
            continue;
        }
        if (mmToDispUnit(size.height() - PageSizeWiggleRoom) > heightInDispUnit) {
            continue;
        }
        m_scanareaPapersize->addItem(QPageSize::name((QPageSize::PageSizeId)sizeCode), size);
    }

    // Add landscape page sizes
    for (int sizeCode: qAsConst(m_sizeCodes)) {
        QSizeF size = QPageSize::size((QPageSize::PageSizeId)sizeCode, QPageSize::Millimeter);
        size.transpose();
        if (mmToDispUnit(size.width() - PageSizeWiggleRoom) > widthInDispUnit) {
            continue;
        }
        if (mmToDispUnit(size.height() - PageSizeWiggleRoom) > heightInDispUnit) {
            continue;
        }
        QString name = QPageSize::name((QPageSize::PageSizeId)sizeCode) +
        i18nc("Page size landscape", " Landscape");
        m_scanareaPapersize->addItem(name , size);
    }

    // Set custom as current
    m_scanareaPapersize->blockSignals(true);
    m_scanareaPapersize->setCurrentIndex(0);
    m_scanareaPapersize->blockSignals(false);
}

void KSaneWidgetPrivate::setPageSize(int)
{
    QSizeF size = m_scanareaPapersize->currentData().toSizeF();
    float pageWidth = mmToDispUnit(size.width());
    float pageHeight = mmToDispUnit(size.height());

    m_settingPageSize = true;
    m_scanareaX->setValue(0);
    m_scanareaY->setValue(0);
    m_scanareaWidth->setValue(pageWidth);
    m_scanareaHeight->setValue(pageHeight);
    m_settingPageSize = false;
}


}  // NameSpace KSaneIface
