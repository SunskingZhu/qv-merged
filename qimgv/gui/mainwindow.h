#pragma once

#include <QApplication>
#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QMimeData>
#include <QImageWriter>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <QDesktopWidget>
#endif

#include "gui/customwidgets/floatingwidgetcontainer.h"
#include "gui/viewers/viewerwidget.h"
#include "gui/overlays/controlsoverlay.h"
#include "gui/overlays/fullscreeninfooverlayproxy.h"
#include "gui/overlays/floatingmessages.h"
#include "gui/overlays/saveconfirmoverlay.h"
#include "gui/panels/mainpanel/thumbnailstrip.h"
#include "gui/panels/sidepanel/sidepanel.h"
#include "gui/panels/croppanel/croppanel.h"
#include "gui/overlays/cropoverlay.h"
#include "gui/overlays/copyoverlay.h"
#include "gui/overlays/changelogwindow.h"
#include "gui/overlays/imageinfooverlayproxy.h"
#include "gui/overlays/renameoverlay.h"
#include "gui/dialogs/resizedialog.h"
#include "gui/centralwidget.h"
#include "gui/dialogs/filereplacedialog.h"
#include "components/actionmanager/actionmanager.h"
#include "settings.h"
#include "gui/dialogs/settingsdialog.h"
#include "gui/viewers/documentwidget.h"
#include "gui/folderview/folderviewproxy.h"
#include "gui/panels/infobar/infobarproxy.h"
#include "gui/overlays/FloatingMessages.h"

#ifdef USE_KDE_BLUR
#include <KWindowEffects>
#endif

struct CurrentInfo {
    int index;
    int fileCount;
    bool slideshow;
    bool shuffle;
		std::shared_ptr<Image> image;
		QFileInfo file_info;
};

enum ActiveSidePanel {
    SIDEPANEL_CROP,
    SIDEPANEL_NONE
};

class MW : public FloatingWidgetContainer
{
    Q_OBJECT
public:
    explicit MW(QWidget *parent = nullptr);
    bool isCropPanelActive();
    void onScalingFinished(std::unique_ptr<QPixmap>scaled);
    void showImage(std::unique_ptr<QPixmap> pixmap);
    void showAnimation(std::shared_ptr<QMovie> movie);
    void showVideo(QString file);

    void setExifInfo(QMap<QString, QString>);
    std::shared_ptr<FolderViewProxy> getFolderView();
    std::shared_ptr<ThumbnailStripProxy> getThumbnailPanel();

		bool imageFits() const;
		bool scaledImageFits() const;
		bool scaledImageWidthFits() const;

    ViewMode currentViewMode();

    bool showConfirmation(QString title, QString msg);
    DialogResult fileReplaceDialog(QString source, QString target, FileReplaceMode mode, bool multiple);

		CurrentInfo &info();
		const CurrentInfo &info() const;

		QIV::FloatingMessage *addPermanentMessage(const QString &text);

private:
    std::shared_ptr<ViewerWidget> viewerWidget;
    QHBoxLayout layout;
    QTimer windowGeometryChangeTimer;
    int currentDisplay;

    bool cropPanelActive, showInfoBarFullscreen, showInfoBarWindowed, maximized;
    std::shared_ptr<DocumentWidget> docWidget;
    std::shared_ptr<FolderViewProxy> folderView;
    std::shared_ptr<CentralWidget> centralWidget;
    ActiveSidePanel activeSidePanel;
    SidePanel *sidePanel;
    CropPanel *cropPanel;
    CropOverlay *cropOverlay;
    SaveConfirmOverlay *saveOverlay;
    ChangelogWindow *changelogWindow;

    CopyOverlay *copyOverlay;

    RenameOverlay *renameOverlay;

    ImageInfoOverlayProxy *imageInfoOverlay;

    ControlsOverlay *controlsOverlay;
    FullscreenInfoOverlayProxy *infoBarFullscreen;
    std::shared_ptr<InfoBarProxy> infoBarWindowed;
    QIV::FloatingMessages *m_floating_messages;

    PanelPosition panelPosition;
    CurrentInfo m_info;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QDesktopWidget desktopWidget;
#endif

    void saveWindowGeometry();
    void restoreWindowGeometry();
    void saveCurrentDisplay();
    void setupUi();

    void applyWindowedBackground();
    void applyFullscreenBackground();
    void mouseDoubleClickEvent(QMouseEvent *event);

    void setupCropPanel();
    void setupCopyOverlay();
    void setupSaveOverlay();
    void setupRenameOverlay();
    void preShowResize(QSize sz);
    void setInteractionEnabled(bool mode);

private slots:
    void updateCurrentDisplay();
    void readSettings();
    void adaptToWindowState();
    void onWindowGeometryChanged();
    void onInfoUpdated();
    void showScriptSettings();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *event);
    void resizeEvent(QResizeEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);

   // bool focusNextPrevChild(bool);
signals:
    void opened(QString);
    void fullscreenStateChanged(bool);
    void copyRequested(QString);
    void moveRequested(QString);
    void copyUrlsRequested(QList<QString>, QString);
    void moveUrlsRequested(QList<QString>, QString);
    void showFoldersChanged(bool);
    void resizeRequested(QSize);
    void renameRequested(QString);
    void cropRequested(QRect);
    void cropAndSaveRequested(QRect);
    void discardEditsRequested();
    void saveAsClicked();
    void saveRequested();
    void saveAsRequested(QString);
    void sortingSelected(SortingMode);

    // viewerWidget
    void scalingRequested(QSize, ScalingFilter);
    void zoomIn();
    void zoomOut();
    void zoomInCursor();
    void zoomOutCursor();
    void scrollUp();
    void scrollDown();
    void scrollLeft();
    void scrollRight();
    void pauseVideo();
    void stopPlayback();
    void seekVideoForward();
    void seekVideoBackward();
    void frameStep();
    void frameStepBack();
    void toggleMute();
    void volumeUp();
    void volumeDown();
    void toggleTransparencyGrid();
    void droppedIn(const QMimeData*, QObject*);
    void draggedOut();
    void setLoopPlayback(bool);
    void playbackFinished();

public slots:
    void setupFullUi();
    void showDefault();
    void showCropPanel();
    void hideCropPanel();
    void toggleFolderView();
    void enableFolderView();
    void enableDocumentView();
    void showOpenDialog(QString path);
    void showSaveDialog(QString filePath);
    QString getSaveFileName(QString fileName);
    void showResizeDialog(QSize initialSize);
    void showSettings();
    void triggerFullScreen();
    void showMessageDirectory(const QString &mame);
    void showMessageDirectoryEnd();
    void showMessageDirectoryStart();

    void showFullScreen();
    void showWindowed();
    void triggerCopyOverlay();
    void showMessage(const QString &text, int duration = 1500);
    void showSuccess(const QString &text);
    void showWarning(const QString &text);
    void showError(const QString &text);
    void triggerMoveOverlay();
    void closeFullScreenOrExit();
    void close();
    void triggerCropPanel();
    void updateCropPanelData();
    void showSaveOverlay();
    void hideSaveOverlay();
    void showChangelogWindow();
    void showChangelogWindow(QString text);
    void fitWindow();
    void fitWidth();
    void fitOriginal();
    void switchFitMode();
    void closeImage();
    void showContextMenu();
    void onSortingChanged(SortingMode);
    void toggleImageInfoOverlay();
    void toggleRenameOverlay(QString currentName);
    void setFilterNearest();
    void setFilterBilinear();
    void setFilter(ScalingFilter filter);
    void toggleScalingFilter();
    void setDirectoryPath(QString path);
    void toggleLockZoom();
    void toggleLockView();
    void toggleFullscreenInfoBar();

		void updateInfo();
};
