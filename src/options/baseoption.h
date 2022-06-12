/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_BASE_OPTION_H
#define KSANE_BASE_OPTION_H

// Qt includes
#include <QObject>

//KDE includes

#include <KLocalizedString>

// Sane includes
extern "C"
{
#include <sane/sane.h>
#include <sane/saneopts.h>
}

#include "../option.h"

#define SANE_TRANSLATION_DOMAIN "sane-backends"

namespace KSaneCore
{

inline QString sane_i18n(const char *text) {
    return i18nd(SANE_TRANSLATION_DOMAIN, text);
}


class BaseOption : public QObject
{
    Q_OBJECT

public:

    BaseOption();
    BaseOption(const SANE_Handle handle, const int index);
    ~BaseOption() override;
    static Option::OptionType optionType(const SANE_Option_Descriptor *optDesc);

    bool needsPolling() const;
    virtual void readOption();
    virtual void readValue();


    virtual QString name() const;
    virtual QString title() const;
    virtual QString description() const;
    virtual Option::OptionType type() const;
    virtual Option::OptionState state() const;
    virtual QVariant minimumValue() const;
    virtual QVariant maximumValue() const;
    virtual QVariant stepValue() const;
    virtual QVariant value() const;
    virtual QVariantList valueList() const;
    virtual QVariantList internalValueList() const;
    virtual Option::OptionUnit valueUnit() const;
    virtual int valueSize() const;
    virtual QString valueAsString() const;

    bool storeCurrentData();
    bool restoreSavedData();

Q_SIGNALS:
    void optionsNeedReload();
    void valuesNeedReload();
    void optionReloaded();
    void valueChanged(const QVariant &value);

public Q_SLOTS:

    virtual bool setValue(const QVariant &value);

protected:

    static SANE_Word toSANE_Word(unsigned char *data);
    static void fromSANE_Word(unsigned char *data, SANE_Word from);
    bool writeData(void *data);
    void beginOptionReload();
    void endOptionReload();

    SANE_Handle                   m_handle = nullptr;
    int                           m_index = -1;
    const SANE_Option_Descriptor *m_optDesc = nullptr; ///< This pointer is provided by sane
    unsigned char                *m_data= nullptr;
    Option::OptionType m_optionType = Option::TypeDetectFail;
};

} // namespace KSaneCore

#endif // KSANE_BASE_OPTION_H

