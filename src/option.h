/*
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_COREOPTION_H
#define KSANE_COREOPTION_H

#include <memory>

// Qt includes

#include "ksanecore_export.h"
#include <QObject>
#include <QString>
#include <QVariant>

namespace KSaneCore
{

class OptionPrivate;

/**
 * A wrapper class providing access to the internal KSaneBaseOption
 * to access all options provided by KSANECore/SANE
 */
class KSANECORE_EXPORT Option : public QObject
{
    Q_OBJECT

public:
    /** This enumeration describes the type of the option. */
    enum OptionType { TypeDetectFail, TypeBool, TypeInteger, TypeDouble, TypeValueList, TypeString, TypeGamma, TypeAction };

    Q_ENUM(OptionType)

    /** This enumeration describes the unit of the value of the option,
     * if any. */
    enum OptionUnit { UnitNone, UnitPixel, UnitBit, UnitMilliMeter, UnitDPI, UnitPercent, UnitMicroSecond, UnitSecond };

    Q_ENUM(OptionUnit)

    /** This enumeration describes the current statue of the value of
     * the option, indicating if this option should be displayed or not. */
    enum OptionState { StateHidden, StateDisabled, StateActive };

    Q_ENUM(OptionState)

    explicit Option(QObject *parent = nullptr);
    ~Option() override;

    /** This function returns the internal name of the option
     * @return the internal name */
    QString name() const;

    /** This function returns the translated title of the option
     * @return the title */
    QString title() const;

    /** This function returns a more verbose, translated description
     * of the option.
     * @return the description */
    QString description() const;

    /** This function the type of the option as determined by KSANECore.
     * Each type provides a different implementation for different
     * variable types, e.g. integer, float or string.
     * @return the type of option the of value OptionType */
    OptionType type() const;

    /** This function returns the state of the option indicating
     * if the function is disabled or should be hidden.
     * @return the state of option the of value OptionState */
    OptionState state() const;

    /** This function returns the currently active value for the option.
     * @return the current value */
    QVariant value() const;

    /** This function returns the minimum value for the option.
     * Returns an empty QVariant if this value is not applicable
     * for the option type.
     * @return the minimum value */
    QVariant minimumValue() const;

    /** This function returns the maximum value for the option.
     * Returns an empty QVariant if this value is not applicable
     * for the option type.
     * @return the maximum value */
    QVariant maximumValue() const;

    /** This function returns the step value for the option.
     * Returns an empty QVariant if this value is not applicable
     * for the option type.
     * @return the step value */
    QVariant stepValue() const;

    /** This function returns the list of possible values if the option
     * is of type OptionType::TypeValueList. The list may contain
     * formatted or translated values.
     * @return a list with all possible values */
    QVariantList valueList() const;

    /** This function returns the list of possible values if the option
     * is of type OptionType::TypeValueList. The list contains the raw
     * internal values without any formatting or translation.
     * @return a list with all possible internal values */
    QVariantList internalValueList() const;

    /** This function returns an enum specifying whether the values
     * of the option have a unit, e.g. mm, px, etc.
     * @return unit of value CoreOptionUnit */
    OptionUnit valueUnit() const;

    /** This function returns the size of the values of the option
     * of type CoreOptionType::TypeValueList.
     * If the size is greater than one, value() and setValue()
     * return and expect a QVariantList with an accordingly number
     * of elements. If the option is a CoreOptionType::TypeString,
     * the size represents the number of characters in the string.
     * @return the number of elements */
    int valueSize() const;

    /** This function temporarily stores the current value
     * in a member variable. */
    bool storeCurrentData();

    /** This function restores the previously saved value
     * and makes it the current value. */
    bool restoreSavedData();

Q_SIGNALS:
    /** This signal is emitted when the option is reloaded, which may
     * happen if the value of other options has changed. */
    void optionReloaded();

    /** This signal is emitted when the current value is updated,
     * either when a user sets a new value or by a reload by the backend. */
    void valueChanged(const QVariant &value);

public Q_SLOTS:

    /** This slot allows to change the current value of the option.
     * @param value the new value of option inside a QVariant.
     * In case the variant cannot be cast to a value suitable for
     * the specific option, the value is discarded. */
    bool setValue(const QVariant &value);

protected:
    std::unique_ptr<KSaneCore::OptionPrivate> d;
};

} // namespace KSaneCore

#endif // KSANE_COREOPTION_H
