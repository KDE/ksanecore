/*
 * SPDX-FileCopyrightText: 2007-2010 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2007 Gilles Caulier <caulier dot gilles at gmail dot com>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2021 Alexander Stippich <a.stippich@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_COREINTERFACE_H
#define KSANE_COREINTERFACE_H

#include "ksanecore_export.h"

#include <memory>

#include <QImage>
#include <QJsonObject>
#include <QList>
#include <QObject>

#include "deviceinformation.h"

namespace KSaneCore
{

class InterfacePrivate;
class Option;

/**
 * This class provides the core interface for accessing the scan controls and options.
 */
class KSANECORE_EXPORT Interface : public QObject
{
    Q_OBJECT
    friend class InterfacePrivate;

public:
    /**
     * Enum defining the message level of the returned scan status string.
     * @note There might come more enumerations in the future.
     */
    enum ScanStatus {
        NoError, // The scanning has finished successfully
        ErrorGeneral, // The error string should contain an error message.
        Information // There is some information to the user.
    };

    /**
     * Enum determining whether the scanner opened correctly.
     */
    enum OpenStatus {
        OpeningSucceeded, // scanner opened successfully
        OpeningDenied, // access was denied,
        OpeningFailed, // opening the scanner failed for unknown reasons
    };

    /**
     * This enumeration is used to obtain a specific option with getOption(KSaneOptionName).
     * Depending on the backend, not all options are available, nor this list is complete.
     * For the remaining options, getOptionsList() must be used.
     */
    enum OptionName {
        SourceOption,
        ScanModeOption,
        BitDepthOption,
        ResolutionOption,
        TopLeftXOption,
        TopLeftYOption,
        BottomRightXOption,
        BottomRightYOption,
        FilmTypeOption,
        NegativeOption,
        InvertColorOption,
        PageSizeOption,
        ThresholdOption,
        XResolutionOption,
        YResolutionOption,
        PreviewOption,
        WaitForButtonOption,
        BrightnessOption,
        ContrastOption,
        GammaOption,
        GammaRedOption,
        GammaGreenOption,
        GammaBlueOption,
        BlackLevelOption,
        WhiteLevelOption,
        BatchModeOption,
        BatchDelayOption,
    };

    /**
     * This enumeration is used to filter the devices found by SANE.
     * Sometimes, webcam may show up as scanner device and some
     * more special scanner are also classified as cameras.
     */
    enum DeviceType { AllDevices, NoCameraAndVirtualDevices };

    /**
     * This constructor initializes the private class variables.
     */
    explicit Interface(QObject *parent = nullptr);

    /**
     * Standard destructor.
     */
    ~Interface() override;

    /**
     * Get the list of available scanning devices. Connect to availableDevices()
     * which is fired once these devices are known.
     * @note While the querying is done in a separate thread and thus not blocking
     * the application, the application must ensure that no other action accessing
     * the scanner device (settings options etc.) is performed during this period.
     * @return whether the devices list are being reloaded or not
     * @param type specify whether only specific device types shall be queried
     */
    bool reloadDevicesList(DeviceType type = AllDevices);

    /**
     * This method opens the specified scanner device and adds the scan options to the
     * options list.
     * @param deviceName is the SANE device name for the scanner to open.
     * @return the status of the opening action.
     */
    OpenStatus openDevice(const QString &deviceName);

    /**
     * This method opens the specified scanner device with a specified username and password.
     * Adds the scan options to the options list.
     * @param deviceName is the SANE device name for the scanner to open.
     * @param userName the username required to open for the scanner.
     * @param password the password required to open for the scanner.
     * @return the status of the opening action.
     */
    OpenStatus openRestrictedDevice(const QString &deviceName, const QString &userName, const QString &password);

    /**
     * This method closes the currently open scanner device.
     * @return 'true' if all goes well and 'false' if no device is open.
     */
    bool closeDevice();

    /**
     * This method returns the internal device name of the currently opened scanner.
     * @note Due to limitations of the SANE API, this will function will return an empty string
     * if reloadDevicesList() has not been called before.
     */
    QString deviceName() const;

    /**
     * This method returns the vendor name of the currently opened scanner.
     * @note Due to limitations of the SANE API, this will function will return an empty string
     * if reloadDevicesList() has not been called before.
     */
    QString deviceVendor() const;

    /**
     * This method returns the model of the currently opened scanner.
     * @note Due to limitations of the SANE API, this will function will return an empty string
     * if reloadDevicesList() has not been called before.
     */
    QString deviceModel() const;

    /**
     * This function is used to set the preferred resolution for scanning the preview.
     * @param dpi is the wanted scan resolution for the preview
     * @note if the set value is not supported, the closest one is used
     * @note setting the value 0 means that the default calculated value should be used.
     * @since 25.04
     */
    void setPreviewResolution(float dpi);

    /**
     * This function returns all available options when a device is opened.
     * @return list containing pointers to all KSaneOptions provided by the backend.
     * Becomes invalid when closing a device.
     * The pointers must not be deleted by the client.
     */
    QList<Option *> getOptionsList();

    /**
     * This function returns a specific option.
     * @param optionEnum the enum specifying the option.
     * @return pointer to the KSaneOption. Returns a nullptr in case the options
     * is not available for the currently opened device.
     */
    Option *getOption(OptionName optionEnum);

    /**
     * This function returns a specific option.
     * @param optionName the internal name of the option defined by SANE.
     * @return pointer to the KSaneOption. Returns a nullptr in case the options
     * is not available for the currently opened device.
     */
    Option *getOption(const QString &optionName);

    /**
     * This method reads the available parameters and their values and
     * returns them in a QMap (Name, value)
     * @return map with the parameter names and their values.
     */
    QMap<QString, QString> getOptionsMap();

    /**
     * This method can be used to write many parameter values at once.
     * @param options a QMap with the parameter names and values.
     * @return This function returns the number of successful writes
     * or -1 if scanning is in progress.
     */
    int setOptionsMap(const QMap<QString, QString> &options);

    /**
     * Gives direct access to the QImage that is used to store the image
     * data retrieved from the scanner.
     * Useful to display an in-progress image while scanning.
     * When accessing the direct image pointer during a scan, the image
     * must be locked before accessing the image and unlocked afterwards
     * using the lockScanImage() and unlockScanImage() functions.
     * @return pointer for direct access of the QImage data.
     */
    QImage *scanImage() const;

    /**
     * Locks the mutex protecting the QImage pointer of scanImage() from
     * concurrent access during scanning.
     */
    void lockScanImage();

    /**
     * Unlocks the mutex protecting the QImage pointer of scanImage() from
     * concurrent access during scanning. The scanning progress will blocked
     * when lockScanImage() is called until unlockScanImage() is called.
     */
    void unlockScanImage();

    /**
     * Returns a JSON object containing the device name, model and vendor.
     * A scanner device must have been opened before, returns an empty
     * object otherwise. Mainly intended for debugging purposes and
     * identifying issues with different scanner hardware.
     * @return JSON object holding the data
     */
    QJsonObject scannerDeviceToJson();

    /**
     * Returns a JSON Object with all available data for all scanner options.
     * A scanner device must have been opened before, returns an empty
     * object otherwise. Mainly intended for debugging purposes and
     * identifying issues with different scanner hardware.
     * @return JSON object holding the data
     */
    QJsonObject scannerOptionsToJson();

public Q_SLOTS:
    /**
     * This method is used to cancel a scan or prevent an automatic new scan.
     */
    void stopScan();

    /**
     * This method is used to start a scan.
     * @note KSaneCore::Interface may return one or more images as a result of one invocation of this slot.
     * If no more images are wanted stopScan() should be called in the slot handling the
     * imageReady signal.
     * @note While scanning is done in a separate thread and thus not blocking
     * the application, the application must ensure that no other action accessing
     * the scanner device (settings options etc.) is performed during this period besides accessing
     * *scanImage() that must be guarded lockScanImage() and unlockScanImage() before and after the access.
     */
    void startScan();

    /**
     * This method is used to start a preview scan.
     * @note KSaneCore::Interface may return one image as a result of one invocation of this slot.
     * The image always covers the full scan area of the device with a reduced resolution.
     * A preferred preview DPI can be set via setPreviewResolution(), but will be adjusted to capabilities
     * of the device.
     * @note While scanning is done in a separate thread and thus not blocking
     * the application, the application must ensure that no other action accessing
     * the scanner device (settings options etc.) is performed during this period besides accessing
     * *scanImage() that must be guarded lockScanImage() and unlockScanImage() before and after the access.
     * @since 25.04
     */
    void startPreviewScan();

Q_SIGNALS:
    /**
     * This signal is emitted when a final scan is ready.
     * @param scannedImage is the QImage containing the scanned image data.
     * @since 25.04
     */
    void scannedImageReady(const QImage &scannedImage);

    /**
     * This signal is emitted when a preview scan is ready.
     * @param scannedImage is the QImage containing the scanned image data of the preview.
     * @since 25.04
     */
    void previewImageReady(const QImage &previewImage);

    /**
     * This signal is emitted when the scanning has ended.
     * @param status contains a ScanStatus status code.
     * @param strStatus If an error has occurred this string will contain an error message.
     * otherwise the string is empty.
     */
    void scanFinished(KSaneCore::Interface::ScanStatus status, const QString &strStatus);

    /**
     * This signal is emitted when the scanning for a preview has ended.
     * @param status contains a ScanStatus status code.
     * @param strStatus If an error has occurred this string will contain an error message.
     * otherwise the string is empty.
     * @since 25.04
     */
    void previewScanFinished(KSaneCore::Interface::ScanStatus status, const QString &strStatus);

    /**
     * This signal is emitted when the user is to be notified about something.
     * @param type contains a ScanStatus code to identify the type of message (error/info/...).
     * @param strStatus If an error has occurred this string will contain an error message.
     * otherwise the string is empty.
     */
    void userMessage(KSaneCore::Interface::ScanStatus status, const QString &strStatus);

    /**
     * This signal is emitted for progress information during a scan.
     * @param percent is the percentage of the scan progress (0-100).
     * A negative value indicates that a scan is being prepared.
     */
    void scanProgress(int percent);

    /**
     * This signal is emitted for progress information during a preview scan.
     * @param percent is the percentage of the preview scan progress (0-100).
     * A negative value indicates that a preview scan is being prepared.
     * @since 25.04
     */
    void previewProgress(int percent);

    /**
     * This signal is emitted every time the device list is updated or
     * after reloadDevicesList() is called.
     * @param deviceList is a QList of KSane::DeviceInformation that contain the
     * device name, model, vendor and type of the attached scanners.
     * @note The list is only a snapshot of the current available devices. Devices
     * might be added or removed/opened after the signal is emitted.
     */
    void availableDevices(const QList<DeviceInformation *> &deviceList);

    /**
     * This signal is emitted when a hardware button is pressed.
     * @param optionName is the untranslated technical name of the sane-option.
     * @param optionLabel is the translated user visible label of the sane-option.
     * @param pressed indicates if the value is true or false.
     * @note The SANE standard does not specify hardware buttons and their behaviors,
     * so this signal is emitted for sane-options that behave like hardware buttons.
     * That is the sane-options are read-only and type boolean. The naming of hardware
     * buttons also differ from backend to backend.
     */
    void buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed);

    /**
     * This signal is emitted for the count down when in batch mode.
     * @param remainingSeconds are the remaining seconds until the next scan starts.
     */
    void batchModeCountDown(int remainingSeconds);

private:
    std::unique_ptr<InterfacePrivate> d;
};

} // namespace KSaneCore

#endif // KSANE_COREINTERFACE_H
