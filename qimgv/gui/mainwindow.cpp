#include "mainwindow.h"
#include "gui/overlays/FloatingMessages.h"

// TODO: nuke this and rewrite

MW::MW(QWidget *parent)
    : FloatingWidgetContainer(parent),
      currentDisplay(0),
      maximized(false),
      activeSidePanel(SIDEPANEL_NONE),
      copyOverlay(nullptr),
      saveOverlay(nullptr),
      renameOverlay(nullptr),
      infoBarFullscreen(nullptr),
      imageInfoOverlay(nullptr),
      m_floating_messages(nullptr),
      cropPanel(nullptr),
      cropOverlay(nullptr)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    layout.setContentsMargins(0,0,0,0);
    layout.setSpacing(0);

    setMinimumSize(10,10);

    // do not steal focus when clicked
    // this is just a container. accept key events only
    // via passthrough from child widgets
    setFocusPolicy(Qt::NoFocus);

    this->setLayout(&layout);

    setWindowTitle(QCoreApplication::applicationName() + " " +
                   QCoreApplication::applicationVersion());

    this->setMouseTracking(true);
    this->setAcceptDrops(true);
    this->setAccessibleName("mainwindow");
    windowGeometryChangeTimer.setSingleShot(true);
    windowGeometryChangeTimer.setInterval(30);
    setupUi();

    connect(settings, &Settings::settingsChanged, this, &MW::readSettings);
    connect(&windowGeometryChangeTimer, &QTimer::timeout, this, &MW::onWindowGeometryChanged);
    connect(this, &MW::fullscreenStateChanged, this, &MW::adaptToWindowState);

    readSettings();
    currentDisplay = settings->lastDisplay();
    maximized = settings->maximizedWindow();
    restoreWindowGeometry();
}

/*                                                             |--[ImageViewer]
 *                        |--[DocumentWidget]--[ViewerWidget]--|
 * [MW]--[CentralWidget]--|                                    |--[VideoPlayer]
 *                        |--[FolderView]
 *
 *  (not counting floating widgets)
 *  ViewerWidget exists for input handling reasons (correct overlay hover handling)
 */
void MW::setupUi() {
    viewerWidget.reset(new ViewerWidget(this));
    infoBarWindowed.reset(new InfoBarProxy(this));
    docWidget.reset(new DocumentWidget(viewerWidget, infoBarWindowed));
    folderView.reset(new FolderViewProxy(this));
    connect(folderView.get(), &FolderViewProxy::sortingSelected, this, &MW::sortingSelected);
    connect(folderView.get(), &FolderViewProxy::directorySelected, this, &MW::opened);
    connect(folderView.get(), &FolderViewProxy::copyUrlsRequested, this, &MW::copyUrlsRequested);
    connect(folderView.get(), &FolderViewProxy::moveUrlsRequested, this, &MW::moveUrlsRequested);
    connect(folderView.get(), &FolderViewProxy::showFoldersChanged, this, &MW::showFoldersChanged);

    centralWidget.reset(new CentralWidget(docWidget, folderView, this));
    layout.addWidget(centralWidget.get());
    controlsOverlay = new ControlsOverlay(docWidget.get());
    infoBarFullscreen = new FullscreenInfoOverlayProxy(viewerWidget.get());
    sidePanel = new SidePanel(this);
    layout.addWidget(sidePanel);
    imageInfoOverlay = new ImageInfoOverlayProxy(viewerWidget.get());
	m_floating_messages = new QIV::FloatingMessages(viewerWidget.get()); // todo: use additional one for folderview?
	m_floating_messages->setMargins(QMargins(16, 16, 16, 16));
    connect(viewerWidget.get(), &ViewerWidget::scalingRequested, this, &MW::scalingRequested);
    connect(viewerWidget.get(), &ViewerWidget::draggedOut, this, qOverload<>(&MW::draggedOut));
    connect(viewerWidget.get(), &ViewerWidget::playbackFinished, this, &MW::playbackFinished);
    connect(viewerWidget.get(), &ViewerWidget::showScriptSettings, this, &MW::showScriptSettings);
    connect(this, &MW::zoomIn,        viewerWidget.get(), &ViewerWidget::zoomIn);
    connect(this, &MW::zoomOut,       viewerWidget.get(), &ViewerWidget::zoomOut);
    connect(this, &MW::zoomInCursor,  viewerWidget.get(), &ViewerWidget::zoomInCursor);
    connect(this, &MW::zoomOutCursor, viewerWidget.get(), &ViewerWidget::zoomOutCursor);
    connect(this, &MW::scrollUp,    viewerWidget.get(), &ViewerWidget::scrollUp);
    connect(this, &MW::scrollDown,  viewerWidget.get(), &ViewerWidget::scrollDown);
    connect(this, &MW::scrollLeft,  viewerWidget.get(), &ViewerWidget::scrollLeft);
    connect(this, &MW::scrollRight, viewerWidget.get(), &ViewerWidget::scrollRight);
    connect(this, &MW::pauseVideo,     viewerWidget.get(), &ViewerWidget::pauseResumePlayback);
    connect(this, &MW::stopPlayback,   viewerWidget.get(), &ViewerWidget::stopPlayback);
    connect(this, &MW::seekVideoForward, viewerWidget.get(), &ViewerWidget::seekForward);
    connect(this, &MW::seekVideoBackward,  viewerWidget.get(), &ViewerWidget::seekBackward);
    connect(this, &MW::frameStep,      viewerWidget.get(), &ViewerWidget::frameStep);
    connect(this, &MW::frameStepBack,  viewerWidget.get(), &ViewerWidget::frameStepBack);
    connect(this, &MW::toggleMute,  viewerWidget.get(), &ViewerWidget::toggleMute);
    connect(this, &MW::volumeUp,  viewerWidget.get(), &ViewerWidget::volumeUp);
    connect(this, &MW::volumeDown,  viewerWidget.get(), &ViewerWidget::volumeDown);
    connect(this, &MW::toggleTransparencyGrid, viewerWidget.get(), &ViewerWidget::toggleTransparencyGrid);
    connect(this, &MW::setLoopPlayback,  viewerWidget.get(), &ViewerWidget::setLoopPlayback);
}

void MW::setupFullUi() {
    setupCropPanel();
    docWidget->allowPanelInit();
    docWidget->setupMainPanel();
    infoBarWindowed->init();
    infoBarFullscreen->init();
}

void MW::setupCropPanel() {
    if(cropPanel)
        return;
    cropOverlay = new CropOverlay(viewerWidget.get());
    cropPanel = new CropPanel(cropOverlay, this);
    connect(cropPanel, &CropPanel::cancel, this, &MW::hideCropPanel);
    connect(cropPanel, &CropPanel::crop,   this, &MW::hideCropPanel);
    connect(cropPanel, &CropPanel::crop,   this, &MW::cropRequested);
    connect(cropPanel, &CropPanel::cropAndSave, this, &MW::hideCropPanel);
    connect(cropPanel, &CropPanel::cropAndSave, this, &MW::cropAndSaveRequested);
}

void MW::setupCopyOverlay() {
    copyOverlay = new CopyOverlay(viewerWidget.get());
    connect(copyOverlay, &CopyOverlay::copyRequested, this, &MW::copyRequested);
    connect(copyOverlay, &CopyOverlay::moveRequested, this, &MW::moveRequested);
}

void MW::setupSaveOverlay() {
    saveOverlay = new SaveConfirmOverlay(viewerWidget.get());
    connect(saveOverlay, &SaveConfirmOverlay::saveClicked,    this, &MW::saveRequested);
    connect(saveOverlay, &SaveConfirmOverlay::saveAsClicked,  this, &MW::saveAsClicked);
    connect(saveOverlay, &SaveConfirmOverlay::discardClicked, this, &MW::discardEditsRequested);
}

void MW::setupRenameOverlay() {
    renameOverlay = new RenameOverlay(this);
    renameOverlay->setName(m_info.file_info.fileName());
    connect(renameOverlay, &RenameOverlay::renameRequested, this, &MW::renameRequested);
}

void MW::toggleFolderView() {
    hideCropPanel();
    if(copyOverlay)
        copyOverlay->hide();
    if(renameOverlay)
        renameOverlay->hide();
    docWidget->hideFloatingPanel();
    imageInfoOverlay->hide();
    centralWidget->toggleViewMode();
    onInfoUpdated();
}

void MW::enableFolderView() {
    hideCropPanel();
    if(copyOverlay)
        copyOverlay->hide();
    if(renameOverlay)
        renameOverlay->hide();
    docWidget->hideFloatingPanel();
    imageInfoOverlay->hide();
    centralWidget->showFolderView();
    onInfoUpdated();
}

void MW::enableDocumentView() {
    centralWidget->showDocumentView();
    onInfoUpdated();
}

ViewMode MW::currentViewMode() {
    return centralWidget->currentViewMode();
}

void MW::fitWindow() {
    if(viewerWidget->interactionEnabled()) {
        viewerWidget->fitWindow();
    } else {
        showMessage("Zoom temporary disabled");
    }
}

void MW::fitWidth() {
    if(viewerWidget->interactionEnabled()) {
        viewerWidget->fitWidth();
    } else {
        showMessage("Zoom temporary disabled");
    }
}

void MW::fitOriginal() {
    if(viewerWidget->interactionEnabled()) {
        viewerWidget->fitOriginal();
    } else {
        showMessage("Zoom temporary disabled");
    }
}

// switch between 1:1 and Fit All
// TODO: move to viewerWidget?
void MW::switchFitMode() {
    if(viewerWidget->fitMode() == FIT_WINDOW)
        viewerWidget->setFitMode(FIT_ORIGINAL);
    else
        viewerWidget->setFitMode(FIT_WINDOW);
}

void MW::closeImage()
{
	m_info.file_info = QFileInfo();
	viewerWidget->closeImage();
}

// todo: fix flicker somehow
// ideally it should change img & resize in one go
void MW::preShowResize(QSize sz) {
    auto screens = qApp->screens();
    if(this->windowState() != Qt::WindowNoState || !screens.count() || screens.count() <= currentDisplay)
        return;
    int decorationSize = frameGeometry().height() - height();
    float maxSzMulti = settings->autoResizeLimit() / 100.f;
    QRect availableGeom = screens.at(currentDisplay)->availableGeometry();
    QSize maxSz = availableGeom.size() * maxSzMulti;
    maxSz.setHeight(maxSz.height() - decorationSize);
    if(!sz.isEmpty()) {
        if(sz.width() > maxSz.width() || sz.height() > maxSz.height())
            sz.scale(maxSz, Qt::KeepAspectRatio);
    } else {
        sz = maxSz;
    }
    QRect newGeom(0,0, sz.width(), sz.height());
    newGeom.moveCenter(availableGeom.center());
    newGeom.translate(0, decorationSize / 2);

    if(this->isVisible())
        setGeometry(newGeom);
    else // setGeometry wont work on hidden windows, so we just save for it to be restored later
        settings->setWindowGeometry(newGeom);
    qApp->processEvents(); // not needed anymore with patched qt?
}

void MW::showImage(std::unique_ptr<QPixmap> pixmap) {
    if(settings->autoResizeWindow())
        preShowResize(pixmap->size());
    viewerWidget->showImage(std::move(pixmap));
    updateCropPanelData();
}

void MW::showAnimation(std::shared_ptr<QMovie> movie) {
    if(settings->autoResizeWindow())
        preShowResize(movie->frameRect().size());
    viewerWidget->showAnimation(movie);
    updateCropPanelData();
}

void MW::showVideo(QString file) {
    if(settings->autoResizeWindow())
        preShowResize(QSize()); // tmp. find a way to get this though mpv BEFORE playback
    viewerWidget->showVideo(file);
}

void MW::showContextMenu() {
    viewerWidget->showContextMenu();
}

void MW::onSortingChanged(SortingMode mode) {
    folderView.get()->onSortingChanged(mode);
    if(centralWidget.get()->currentViewMode() == ViewMode::MODE_DOCUMENT) {
        switch(mode) {
            case SortingMode::SORT_NAME_ASC:      showMessage("Sorting: By Name");              break;
            case SortingMode::SORT_NAME_DESC: showMessage("Sorting: By Name (desc.)");      break;
            case SortingMode::SORT_TIME_ASC:      showMessage("Sorting: By Time");              break;
            case SortingMode::SORT_TIME_DESC: showMessage("Sorting: By Time (desc.)");      break;
            case SortingMode::SORT_SIZE_ASC:      showMessage("Sorting: By File Size");         break;
            case SortingMode::SORT_SIZE_DESC: showMessage("Sorting: By File Size (desc.)"); break;
        }
    }
}

void MW::setDirectoryPath(QString path)
{
	//m_info.file_info = QFileInfo(path);
    //closeImage();
  folderView->setDirectoryPath(path);
 // onInfoUpdated();
}

void MW::toggleLockZoom() {
    viewerWidget->toggleLockZoom();
    if(viewerWidget->lockZoomEnabled())
        showMessage("Zoom lock: ON");
    else
        showMessage("Zoom lock: OFF");
    onInfoUpdated();
}

void MW::toggleLockView() {
    viewerWidget->toggleLockView();
    if(viewerWidget->lockViewEnabled())
        showMessage("View lock: ON");
    else
        showMessage("View lock: OFF");
    onInfoUpdated();
}

void MW::toggleFullscreenInfoBar() {
    //if(!this->isFullScreen())
    //    return;
    showInfoBarFullscreen = !showInfoBarFullscreen;
    if(showInfoBarFullscreen)
        infoBarFullscreen->showWhenReady();
    else
			infoBarFullscreen->hide();
}

void MW::updateInfo()
{
	onInfoUpdated();
}

void MW::toggleImageInfoOverlay() {
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;
    if(imageInfoOverlay->isHidden())
        imageInfoOverlay->show();
    else
        imageInfoOverlay->hide();
}

void MW::toggleRenameOverlay(QString currentName) {
    if(!renameOverlay)
        setupRenameOverlay();
    if(renameOverlay->isHidden()) {
        renameOverlay->setBackdropEnabled((centralWidget->currentViewMode() == MODE_FOLDERVIEW));
        renameOverlay->setName(currentName);
        renameOverlay->show();
    } else {
        renameOverlay->hide();
    }
}

void MW::toggleScalingFilter() {
    ScalingFilter configuredFilter = settings->scalingFilter();
    if(viewerWidget->scalingFilter() == configuredFilter) {
        setFilterNearest();
    }
    else {
        setFilter(configuredFilter);
    }
}

void MW::setFilterNearest() {
    showMessage("Filter: nearest", 600);
    viewerWidget->setFilterNearest();
}

void MW::setFilterBilinear() {
    showMessage("Filter: bilinear", 600);
    viewerWidget->setFilterBilinear();
}

void MW::setFilter(ScalingFilter filter) {
    QString filterName;
    switch (filter) {
        case QI_FILTER_NEAREST:
            filterName = "nearest";
            break;
        case ScalingFilter::QI_FILTER_BILINEAR:
            filterName = "bilinear";
            break;
        case QI_FILTER_CV_BILINEAR_SHARPEN:
            filterName = "bilinear + sharpen";
            break;
        case QI_FILTER_CV_CUBIC:
            filterName = "bicubic";
            break;
        case QI_FILTER_CV_CUBIC_SHARPEN:
            filterName = "bicubic + sharpen";
            break;
			case QI_FILTER_CV_LANCZOS4:
					filterName = "lanczos4";
					break;
			case QI_FILTER_CV_LANCZOS4_SHARPEN:
					filterName = "lanczos4 + sharpen";
					break;
        default:
            filterName = "configured " + QString::number(static_cast<int>(filter));
            break;
    }
    showMessage("Filter " + filterName, 600);
    viewerWidget->setScalingFilter(filter);
}

bool MW::isCropPanelActive() {
    return (activeSidePanel == SIDEPANEL_CROP);
}

void MW::onScalingFinished(std::unique_ptr<QPixmap> scaled) {
    viewerWidget->onScalingFinished(std::move(scaled));
}

void MW::saveWindowGeometry() {
    if(this->windowState() == Qt::WindowNoState)
        settings->setWindowGeometry(geometry());
    settings->setMaximizedWindow(maximized);
}

// does not apply fullscreen; window size / maximized state only
void MW::restoreWindowGeometry() {
    this->setGeometry(settings->windowGeometry());
    if(settings->maximizedWindow())
        this->setWindowState(Qt::WindowMaximized);
    updateCurrentDisplay();
}

void MW::updateCurrentDisplay() {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    currentDisplay = desktopWidget.screenNumber(this);
#else
    auto screens = qApp->screens();
    currentDisplay = screens.indexOf(this->window()->screen());
#endif
}

void MW::onWindowGeometryChanged() {
    saveWindowGeometry();
    updateCurrentDisplay();
}

void MW::saveCurrentDisplay() {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    settings->setLastDisplay(desktopWidget.screenNumber(this));
#else
    settings->setLastDisplay(qApp->screens().indexOf(this->window()->screen()));
#endif
}

//#############################################################
//######################### EVENTS ############################
//#############################################################

void MW::mouseMoveEvent(QMouseEvent *event) {
    event->ignore();
}

bool MW::event(QEvent *event) {
    // only save maximized state if we are already visible
    // this filter out out events while the window is still being set up
    if(event->type() == QEvent::WindowStateChange && this->isVisible() && !this->isFullScreen())
        maximized = isMaximized();
    if(event->type() == QEvent::Move || event->type() == QEvent::Resize)
        windowGeometryChangeTimer.start();
    return QWidget::event(event);
}

// hook up to actionManager
void MW::keyPressEvent(QKeyEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::wheelEvent(QWheelEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::mousePressEvent(QMouseEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::mouseReleaseEvent(QMouseEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::mouseDoubleClickEvent(QMouseEvent *event) {
    event->accept();
    QMouseEvent *fakePressEvent = new QMouseEvent(QEvent::MouseButtonPress, event->pos(), event->button(), event->buttons(), event->modifiers());
    actionManager->processEvent(fakePressEvent);
    actionManager->processEvent(event);
}

void MW::close() {
    saveWindowGeometry();
    saveCurrentDisplay();
    // try to close window sooner
    // since qt6.3 QWidget::close() no longer works on hidden windows (bug?)
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    this->hide();
#endif
    if(copyOverlay)
        copyOverlay->saveSettings();
    QWidget::close();
}

void MW::closeEvent(QCloseEvent *event) {
    // catch the close event when user presses X on the window itself
    event->accept();
    actionManager->invokeAction("exit");
}

void MW::dragEnterEvent(QDragEnterEvent *e) {
    if(e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MW::dropEvent(QDropEvent *event) {
    emit droppedIn(event->mimeData(), event->source());
}

void MW::resizeEvent(QResizeEvent *event) {
    if(activeSidePanel == SIDEPANEL_CROP) {
        cropOverlay->setImageScale(viewerWidget->currentScale());
        cropOverlay->setImageDrawRect(viewerWidget->imageRect());
    }
    FloatingWidgetContainer::resizeEvent(event);
}

void MW::showDefault() {
    if(!this->isVisible()) {
        if(settings->fullscreenMode())
            showFullScreen();
        else
            showWindowed();
    }
}

void MW::showSaveDialog(QString filePath) {
    QString newFilePath = getSaveFileName(filePath);
    if(!newFilePath.isEmpty())
        emit saveAsRequested(newFilePath);
}

QString MW::getSaveFileName(QString filePath) {
    docWidget->hideFloatingPanel();
    QStringList filters;
    // generate filter for writable images
    // todo: some may need to be blacklisted
    auto writerFormats = QImageWriter::supportedImageFormats();
    if(writerFormats.contains("jpg"))  filters.append("JPEG (*.jpg *.jpeg *jpe *jfif)");
    if(writerFormats.contains("png"))  filters.append("PNG (*.png)");
    if(writerFormats.contains("webp")) filters.append("WebP (*.webp)");
    // may not work..
    if(writerFormats.contains("jp2"))  filters.append("JPEG 2000 (*.jp2 *.j2k *.jpf *.jpx *.jpm *.jpgx)");
    if(writerFormats.contains("jxl"))  filters.append("JPEG-XL (*.jxl)");
    if(writerFormats.contains("avif")) filters.append("AVIF (*.avif *.avifs)");
    if(writerFormats.contains("tif"))  filters.append("TIFF (*.tif *.tiff)");
    if(writerFormats.contains("bmp"))  filters.append("BMP (*.bmp)");
#ifdef _WIN32
    if(writerFormats.contains("ico"))  filters.append("Icon Files (*.ico)");
#endif
    if(writerFormats.contains("ppm"))  filters.append("PPM (*.ppm)");
    if(writerFormats.contains("xbm"))  filters.append("XBM (*.xbm)");
    if(writerFormats.contains("xpm"))  filters.append("XPM (*.xpm)");
    if(writerFormats.contains("dds"))  filters.append("DDS (*.dds)");
    if(writerFormats.contains("wbmp")) filters.append("WBMP (*.wbmp)");
    // add everything else from imagewriter
    for(auto fmt : writerFormats) {
        if(filters.filter(fmt).isEmpty())
            filters.append(fmt.toUpper() + " (*." + fmt + ")");
    }
    QString filterString = filters.join(";; ");

    // find matching filter for the current image
    QString selectedFilter = "JPEG (*.jpg *.jpeg *jpe *jfif)";
    QFileInfo fi(filePath);
    for(auto filter : filters) {
        if(filter.contains(fi.suffix().toLower())) {
            selectedFilter = filter;
            break;
        }
    }
    QString newFilePath = QFileDialog::getSaveFileName(this, tr("Save File as..."), filePath, filterString, &selectedFilter);
    return newFilePath;
}

void MW::showOpenDialog(QString path) {
    docWidget->hideFloatingPanel();

    QFileDialog dialog(this);
    QStringList imageFilter;
    imageFilter.append(settings->supportedFormatsFilter());
    imageFilter.append("All Files (*)");
    dialog.setDirectory(path);
    dialog.setNameFilters(imageFilter);
    dialog.setWindowTitle("Open image");
    dialog.setWindowModality(Qt::ApplicationModal);
    connect(&dialog, &QFileDialog::fileSelected, this, &MW::opened);
    dialog.exec();
}

void MW::showResizeDialog(QSize initialSize) {
    ResizeDialog dialog(initialSize, this);
    connect(&dialog, &ResizeDialog::sizeSelected, this, &MW::resizeRequested);
    dialog.exec();
}

DialogResult MW::fileReplaceDialog(QString src, QString dst, FileReplaceMode mode, bool multiple) {
    FileReplaceDialog dialog(this);
    dialog.setModal(true);
    dialog.setSource(src);
    dialog.setDestination(dst);
    dialog.setMode(mode);
    dialog.setMulti(multiple);

    dialog.exec();

		return dialog.getResult();
}

CurrentInfo &MW::info()
{
	return m_info;
}

const CurrentInfo &MW::info() const
{
	return m_info;
}

void MW::showSettings() {
    docWidget->hideFloatingPanel();
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

void MW::showScriptSettings() {
    docWidget->hideFloatingPanel();
    SettingsDialog settingsDialog(this);
    settingsDialog.switchToPage(4);
    settingsDialog.exec();
}

void MW::triggerFullScreen() {
    if(!isFullScreen()) {
        showFullScreen();
    } else {
        showWindowed();
    }
}

void MW::showFullScreen() {
    //do not save immediately on application start
    if(!isHidden())
        saveWindowGeometry();
    auto screens = qApp->screens();
    // todo: why check the screen again?
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    int _currentDisplay = desktopWidget.screenNumber(this);
#else
    int _currentDisplay = screens.indexOf(this->window()->screen());
#endif
    //move to target screen
    if(screens.count() > currentDisplay && currentDisplay != _currentDisplay) {
        this->move(screens.at(currentDisplay)->geometry().x(),
                   screens.at(currentDisplay)->geometry().y());
    }
    QWidget::showFullScreen();
    // try to repaint sooner
    qApp->processEvents();
    emit fullscreenStateChanged(true);
}

void MW::showWindowed() {
    if(isFullScreen())
        QWidget::showNormal();
    restoreWindowGeometry();
    QWidget::show();
    // try to repaint sooner
    qApp->processEvents();
    emit fullscreenStateChanged(false);
}

void MW::updateCropPanelData() {
    if(cropPanel && activeSidePanel == SIDEPANEL_CROP) {
        cropPanel->setImageRealSize(viewerWidget->sourceSize());
        cropOverlay->setImageDrawRect(viewerWidget->imageRect());
        cropOverlay->setImageScale(viewerWidget->currentScale());
        cropOverlay->setImageRealSize(viewerWidget->sourceSize());
    }
}

void MW::showSaveOverlay() {
    if(!settings->showSaveOverlay())
        return;
    if(!saveOverlay)
        setupSaveOverlay();
    saveOverlay->show();
}

void MW::hideSaveOverlay() {
    if(!saveOverlay)
        return;
    saveOverlay->hide();
}

void MW::showChangelogWindow() {
    changelogWindow->show();
}

void MW::showChangelogWindow(QString text) {
    changelogWindow->setText(text);
    changelogWindow->show();
}

void MW::triggerCropPanel() {
    if(activeSidePanel != SIDEPANEL_CROP) {
        showCropPanel();
    } else {
        hideCropPanel();
    }
}

void MW::showCropPanel() {
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;

    if(activeSidePanel != SIDEPANEL_CROP) {
        docWidget->hideFloatingPanel();
        sidePanel->setWidget(cropPanel);
        sidePanel->show();
        cropOverlay->show();
        activeSidePanel = SIDEPANEL_CROP;
        // reset & lock zoom so CropOverlay won't go crazy
        viewerWidget->fitWindow();
        setInteractionEnabled(false);
        // feed the panel current image info
        updateCropPanelData();
    }
}

void MW::setInteractionEnabled(bool mode) {
    docWidget->setInteractionEnabled(mode);
    viewerWidget->setInteractionEnabled(mode);
}

void MW::hideCropPanel() {
    sidePanel->hide();
    if(activeSidePanel == SIDEPANEL_CROP) {
        cropOverlay->hide();
        setInteractionEnabled(true);
    }
    activeSidePanel = SIDEPANEL_NONE;
}

void MW::triggerCopyOverlay() {
    if(!viewerWidget->isDisplaying())
        return;
    if(!copyOverlay)
        setupCopyOverlay();

    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;
    if(copyOverlay->operationMode() == OVERLAY_COPY) {
        copyOverlay->isHidden() ? copyOverlay->show() : copyOverlay->hide();
    } else {
        copyOverlay->setDialogMode(OVERLAY_COPY);
        copyOverlay->show();
    }
}

void MW::triggerMoveOverlay() {
    if(!viewerWidget->isDisplaying())
        return;
    if(!copyOverlay)
        setupCopyOverlay();

    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;
    if(copyOverlay->operationMode() == OVERLAY_MOVE) {
        copyOverlay->isHidden() ? copyOverlay->show() : copyOverlay->hide();
    } else {
        copyOverlay->setDialogMode(OVERLAY_MOVE);
        copyOverlay->show();
    }
}

// quit fullscreen or exit the program
void MW::closeFullScreenOrExit() {
    if(this->isFullScreen()) {
        this->showWindowed();
    } else {
        actionManager->invokeAction("exit");
    }
}

void MW::onInfoUpdated()
{
	if (renameOverlay) {
		renameOverlay->setName(m_info.file_info.fileName());
	}

	QFileInfo file_info = m_info.file_info;

	bool file_opened = false;
	if (centralWidget->currentViewMode() == MODE_FOLDERVIEW) {
		setWindowTitle(tr("Folder view"));
	} else if (file_info.size() == 0) {
		setWindowTitle(qApp->applicationName());
	} else {
		file_opened = true;
	}

	if (file_opened == false) {
		infoBarFullscreen->setInfo({tr("No file opened.")});
		infoBarWindowed->setInfo("", tr("No file opened."), "");
		return;
	}

	QString position = m_info.fileCount ? QStringLiteral("[%1/%2]").arg(m_info.index + 1).arg(m_info.fileCount) : QString();

	QString size = file_info.size() ? this->locale().formattedDataSize(file_info.size(), 1) : QString();

	Image* image = m_info.image.get();
	QString format = image ? image->format().toUpper() : QString();
	QString mime = image ? image->mimeType().name().toUtf8() : QString();

	QSize image_size = image ? image->size() : QSize();
	QString resolution = image_size.width() ? QStringLiteral("%1 x %2").arg(image_size.width()).arg(image_size.height()) : QString();

	QString filename = file_info.fileName();
	if (image && image->isEdited()) {
		filename += "*";
	}

	// toggleable states
	QStringList states;
	if (m_info.slideshow) {
		states << "[slideshow]";
	}
	if (m_info.shuffle) {
		states << "[shuffle]";
	}
	if (viewerWidget->lockZoomEnabled()) {
		states << "[zoom lock]";
	}
	if (viewerWidget->lockViewEnabled()) {
		states << "[view lock]";
	}

	{
		QStringList title;

		title << QStringLiteral("%1 > %2 %3").arg(file_info.dir().dirName(), filename, position);

		if (settings->windowTitleExtendedInfo()) {
			if (resolution.isEmpty() == false) {
				title << resolution;
			}
			if (size.isEmpty() == false) {
				title << size;
			}
		}

		title << qApp->applicationName();

		setWindowTitle(title.join(" :: "));
	}

	// if(!settings->infoBarWindowed() && !states.isEmpty())
	// 		windowTitle.append(" -" + states);

	QStringList info;
	info << file_info.absolutePath();

	QStringList image_info;
	if (resolution.isEmpty() == false) {
		image_info << resolution;
	}
	if (size.isEmpty() == false) {
		image_info << size;
	}
	if (format.isEmpty() == false) {
		image_info << format;
	}
	if (mime.isEmpty() == false) {
		image_info << mime;
	}
	if (states.isEmpty() == false) {
		image_info << states;
	}

	info << filename << image_info.join(" :: ") << file_info.lastModified().toString("yyyy-MM-dd HH:mm:ss");

	if (m_info.fileCount) {
		info << QStringLiteral("%1 / %2").arg(m_info.index + 1).arg(m_info.fileCount);
	}

	infoBarFullscreen->setInfo(info);

	// if (showInfoBarFullscreen == false) {
	// 	infoBarFullscreen->showWhenReady(1500);
	// }

	infoBarWindowed->setInfo(position, filename, resolution + "  " + size + " " + states.join(' '));
}

// TODO!!! buffer this in mw
void MW::setExifInfo(QMap<QString, QString> info) {
    if(imageInfoOverlay)
        imageInfoOverlay->setExifInfo(info);
}

std::shared_ptr<FolderViewProxy> MW::getFolderView() {
    return folderView;
}

std::shared_ptr<ThumbnailStripProxy> MW::getThumbnailPanel() {
	return docWidget->thumbPanel();
}

bool MW::imageFits() const
{
	return viewerWidget->imageFits();
}

bool MW::scaledImageFits() const
{
	return viewerWidget->scaledImageFits();
}

bool MW::scaledImageWidthFits() const
{
	return viewerWidget->scaledImageWidthFits();
}

QIV::FloatingMessage *MW::addPermanentMessage(const QString &text)
{
	return m_floating_messages->addMessage(text, QIV::FloatingMessageIcon::NO_ICON, QIV::FloatingMessages::DURATION_FOREVER);
}

// todo: this is crap
void MW::showMessageDirectory(const QString &mame)
{
	m_floating_messages->addMessage(mame, QIV::FloatingMessageIcon::ICON_DIRECTORY);
}

void MW::showMessageDirectoryEnd()
{
	m_floating_messages->addMessage(tr("The last image in the directory"), QIV::FloatingMessageIcon::ICON_RIGHT_EDGE);
    // TODO replace with something nicer (integrate with click overlay?)
    //m_floating_messages->showMessage("", FloatingWidgetPosition::RIGHT, FloatingMessageIcon::ICON_RIGHT_EDGE, 400);
}

void MW::showMessageDirectoryStart()
{
	m_floating_messages->addMessage(tr("The first image in the directory"), QIV::FloatingMessageIcon::ICON_LEFT_EDGE);
    // TODO replace with something nicer (integrate with click overlay?)
    //m_floating_messages->showMessage("", FloatingWidgetPosition::LEFT, FloatingMessageIcon::ICON_LEFT_EDGE, 400);
}

void MW::showMessage(const QString &text, int duration)
{
	m_floating_messages->addMessage(text, QIV::FloatingMessageIcon::NO_ICON, duration);
}

void MW::showSuccess(const QString &text)
{
	m_floating_messages->success(text);
}

void MW::showWarning(const QString &text)
{
	m_floating_messages->warning(text);
}

void MW::showError(const QString &text)
{
	m_floating_messages->error(text);
}

bool MW::showConfirmation(QString title, QString msg) {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(msg);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setModal(true);
    if(msgBox.exec() == QMessageBox::Yes)
        return true;
    else
        return false;
}

void MW::readSettings() {
    showInfoBarFullscreen = settings->infoBarFullscreen();
    showInfoBarWindowed = settings->infoBarWindowed();
    adaptToWindowState();
}

// todo: remove/rename?
void MW::applyWindowedBackground() {
#ifdef USE_KDE_BLUR
    if(settings->backgroundOpacity() == 1.0)
        KWindowEffects::enableBlurBehind(winId(), false);
    else
        KWindowEffects::enableBlurBehind(winId(), settings->blurBackground());
#endif
}

void MW::applyFullscreenBackground() {
#ifdef USE_KDE_BLUR
    KWindowEffects::enableBlurBehind(winId(), false);
#endif
}

// changes ui elements according to fullscreen state
void MW::adaptToWindowState() {
    docWidget->hideFloatingPanel();
    if(isFullScreen()) { //-------------------------------------- fullscreen ---
        applyFullscreenBackground();
        infoBarWindowed->hide();

        if(showInfoBarFullscreen)
            infoBarFullscreen->showWhenReady();
        else
            infoBarFullscreen->hide();

        auto pos = settings->panelPosition();
        if(!settings->panelEnabled() || pos == PANEL_BOTTOM || pos == PANEL_LEFT)
            controlsOverlay->show();
        else
            controlsOverlay->hide();
    } else { //------------------------------------------------------ window ---
        applyWindowedBackground();
		    if(showInfoBarFullscreen)
			    infoBarFullscreen->showWhenReady();
		    else
			    infoBarFullscreen->hide();

        if(showInfoBarWindowed)
            infoBarWindowed->show();
        else
            infoBarWindowed->hide();

        controlsOverlay->hide();
    }
    folderView->onFullscreenModeChanged(isFullScreen());
    docWidget->onFullscreenModeChanged(isFullScreen());
    viewerWidget->onFullscreenModeChanged(isFullScreen());
}

void MW::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    FloatingWidgetContainer::paintEvent(event);
}

void MW::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    docWidget->hideFloatingPanel(true);
}

// block native tab-switching so we can use it in shortcuts
//bool MW::focusNextPrevChild(bool) {
//    return false;
//}
