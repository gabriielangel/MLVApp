/*!
 * \file MainWindow.h
 * \author masc4ii
 * \copyright 2017
 * \brief The main window
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QDebug>
#include <QTimerEvent>
#include <QResizeEvent>
#include <QFileOpenEvent>
#include <QThreadPool>
#include <QProcess>
#include <QVector>
#include <QGraphicsPixmapItem>
#include "../../src/mlv_include.h"
#include "InfoDialog.h"
#include "StatusDialog.h"
#include "Histogram.h"
#include "WaveFormMonitor.h"
#include "AudioWave.h"
#include "ReceiptSettings.h"
#include "AudioPlayback.h"
#include "GraphicsPickerScene.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int &argc, char **argv, QWidget *parent = 0);
    ~MainWindow();

protected:
    void timerEvent( QTimerEvent *t );
    void resizeEvent( QResizeEvent *event );
    bool event( QEvent *event );
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private slots:
    void on_actionOpen_triggered();
    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();
    void on_horizontalSliderPosition_valueChanged(int position);
    void on_actionClip_Information_triggered();
    void on_horizontalSliderExposure_valueChanged(int position);
    void on_horizontalSliderTemperature_valueChanged(int position);
    void on_horizontalSliderTint_valueChanged(int position);
    void on_horizontalSliderSaturation_valueChanged(int position);
    void on_horizontalSliderDS_valueChanged(int position);
    void on_horizontalSliderDR_valueChanged(int position);
    void on_horizontalSliderLS_valueChanged(int position);
    void on_horizontalSliderLR_valueChanged(int position);
    void on_horizontalSliderLighten_valueChanged(int position);
    void on_horizontalSliderSharpen_valueChanged(int position);
    void on_horizontalSliderChromaBlur_valueChanged(int position);
    void on_actionGoto_First_Frame_triggered();
    void on_actionExport_triggered();
    void on_actionExportActualFrame_triggered();
    void on_checkBoxHighLightReconstruction_toggled(bool checked);
    void on_checkBoxChromaSeparation_toggled(bool checked);
    void on_comboBoxProfile_currentIndexChanged(int index);
    void on_actionZoomFit_triggered(bool on);
    void on_actionZoom100_triggered();
    void on_actionShowHistogram_triggered(void);
    void on_actionShowWaveFormMonitor_triggered(void);
    void on_actionShowParade_triggered(void);
    void on_actionAlwaysUseAMaZE_triggered(bool checked);
    void on_actionExportSettings_triggered();
    void on_actionResetReceipt_triggered();
    void on_actionCopyRecept_triggered();
    void on_actionPasteReceipt_triggered();
    void on_actionNewSession_triggered();
    void on_actionOpenSession_triggered();
    void on_actionSaveSession_triggered();
    void on_actionCaching_triggered( bool checked );
    void readFFmpegOutput( void );
    void endExport( void );
    void on_listWidgetSession_activated(const QModelIndex &index);
    void on_dockWidgetSession_visibilityChanged(bool visible);
    void on_dockWidgetEdit_visibilityChanged(bool visible);
    void on_actionShowAudioTrack_triggered(bool checked);
    void on_listWidgetSession_customContextMenuRequested(const QPoint &pos);
    void deleteFileFromSession( void );
    void rightClickShowFile( void );
    void selectAllFiles( void );
    void pictureCustomContextMenuRequested(const QPoint &pos);
    void on_labelHistogram_customContextMenuRequested(const QPoint &pos);
    void on_label_ExposureVal_doubleClicked( void );
    void on_label_TemperatureVal_doubleClicked( void );
    void on_label_TintVal_doubleClicked( void );
    void on_label_SaturationVal_doubleClicked( void );
    void on_label_DrVal_doubleClicked( void );
    void on_label_DsVal_doubleClicked( void );
    void on_label_LrVal_doubleClicked( void );
    void on_label_LsVal_doubleClicked( void );
    void on_label_LightenVal_doubleClicked( void );
    void on_label_Sharpen_doubleClicked( void );
    void on_label_ChromaBlur_doubleClicked( void );
    void on_labelAudioTrack_sizeChanged( void );
    void on_actionFullscreen_triggered(bool checked);
    void exportHandler( void );
    void on_actionPlay_triggered(bool checked);
    void on_actionShowZebras_triggered();
    void on_comboBoxFocusPixelSwitch_currentIndexChanged(int index);
    void on_comboBoxFocusPixelsInterpolationMethod_currentIndexChanged(int index);
    void on_comboBoxBadPixelsSwitch_currentIndexChanged(int index);
    void on_comboBoxBadPixelsInterpolationMethod_currentIndexChanged(int index);
    void on_comboBoxChromaSmoothSwitch_currentIndexChanged(int index);
    void on_comboBoxPatternNoiseSwitch_currentIndexChanged(int index);
    void on_comboBoxVerticalStripesSwitch_currentIndexChanged(int index);
    void on_spinBoxDeflickerTarget_valueChanged(int arg1);
    void on_comboBoxDualISO_currentIndexChanged(int index);
    void on_comboBoxDualISOInterpolation_currentIndexChanged(int index);
    void on_comboBoxDualISOAliasMap_currentIndexChanged(int index);
    void on_comboBoxDualISOFullresBlending_currentIndexChanged(int index);
    void on_actionNextFrame_triggered();
    void on_actionPreviousFrame_triggered();
    void on_checkBoxRawFixEnable_clicked(bool checked);
    void on_actionWhiteBalancePicker_toggled(bool checked);
    void whiteBalancePicked( int x, int y );


private:
    Ui::MainWindow *ui;
    InfoDialog *m_pInfoDialog;
    StatusDialog *m_pStatusDialog;
    Histogram *m_pHistogram;
    WaveFormMonitor *m_pWaveFormMonitor;
    AudioWave *m_pAudioWave;
    AudioPlayback *m_pAudioPlayback;
    mlvObject_t *m_pMlvObject;
    processingObject_t *m_pProcessingObject;
    QGraphicsPixmapItem *m_pGraphicsItem;
    GraphicsPickerScene* m_pScene;
    QLabel *m_pCachingStatus;
    QLabel *m_pFpsStatus;
    QLabel *m_pFrameNumber;
    uint8_t *m_pRawImage;
    uint32_t m_cacheSizeMB;
    uint8_t m_codecProfile;
    uint8_t m_codecOption;
    uint8_t m_previewMode;
    bool m_frameChanged;
    int m_currentFrameIndex;
    double m_newPosDropMode;
    bool m_dontDraw;
    bool m_frameStillDrawing;
    bool m_fileLoaded;
    bool m_inOpeningProcess;
    int m_timerId;
    int m_timerCacheId;
    bool m_fpsOverride;
    bool m_tryToSyncAudio;
    bool m_audioExportEnabled;
    double m_frameRate;
    QString m_lastSaveFileName;
    QProcess *m_pFFmpeg;
    ReceiptSettings *m_pReceiptClipboard;
    QVector<ReceiptSettings*> m_pSessionReceipts;
    QVector<ReceiptSettings*> m_exportQueue;
    int m_lastActiveClipInSession;
    int m_styleSelection;
    void drawFrame( void );
    void openMlv( QString fileName );
    void playbackHandling( int timeDiff );
    void initGui( void );
    void initLib( void );
    void readSettings( void );
    void writeSettings( void );
    void startExport( QString fileName );
    void startExportPipe( QString fileName );
    void addFileToSession( QString fileName );
    void openSession( QString fileName );
    void saveSession( QString fileName );
    void deleteSession( void );
    bool isFileInSession( QString fileName );
    void setSliders( ReceiptSettings *sliders );
    void setReceipt( ReceiptSettings *sliders );
    void showFileInEditor( int row );
    void addClipToExportQueue( int row, QString fileName );
    void previewPicture( int row );
    void setPreviewMode( void );
    double getFramerate( void );
    void paintAudioTrack( void );
    void drawZebras( void );
    void drawFrameNumberLabel( void );
    void setDualIsoIfDetected( void );

signals:
    void exportReady( void );
};

class RenderPngTask : public QRunnable
{
public:
    RenderPngTask( mlvObject_t *pMlvObject, QString fileName, uint32_t frame = 0 )
    {
        m_frame = frame;
        m_pMlvObject = pMlvObject;
        m_fileName = fileName;
    }
private:
    void run();

    uint32_t m_frame;
    QString m_fileName;
    mlvObject_t *m_pMlvObject;
};

#endif // MAINWINDOW_H
