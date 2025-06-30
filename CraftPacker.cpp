#include "CraftPacker.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QUrlQuery>
#include <QSplitter>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QMenuBar>
#include <QMenu>
#include <QDirIterator>
#include <QDebug>

const QString STYLESHEET = R"( QMainWindow, QDialog { background-color: #2c3e50; color: #ecf0f1; } QMenuBar { background-color: #34495e; color: #ecf0f1; } QMenuBar::item:selected { background-color: #3498db; } QMenu { background-color: #34495e; border: 1px solid #7f8c8d; } QMenu::item:selected { background-color: #3498db; } QGroupBox { border: 1px solid #7f8c8d; border-radius: 5px; margin-top: 1ex; font-weight: bold; color: #ecf0f1; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 3px; background-color: #2c3e50; } QLabel, QCheckBox { color: #ecf0f1; font-size: 10pt; } QLineEdit, QTextEdit, QComboBox, QListWidget, QSpinBox { background-color: #34495e; color: #ecf0f1; border: 1px solid #7f8c8d; border-radius: 4px; padding: 5px; font-size: 10pt; } QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QListWidget:focus, QSpinBox:focus { border: 1px solid #3498db; } QListWidget::item:hover { background-color: #3a5064; } QListWidget::item:selected { background-color: #3498db; color: white; } QComboBox::drop-down { border: none; } QComboBox::down-arrow { image: url(nul); } QPushButton { background-color: #3498db; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-size: 10pt; font-weight: bold; } QPushButton:hover { background-color: #2980b9; } QPushButton:pressed { background-color: #1f618d; } QPushButton:disabled { background-color: #566573; color: #95a5a6; } QTreeWidget { background-color: #34495e; color: #ecf0f1; border: 1px solid #7f8c8d; border-radius: 4px; alternate-background-color: #3a5064; } QTreeWidget::item { padding: 4px; } QHeaderView::section { background-color: #3a5064; color: white; padding: 4px; border: 1px solid #7f8c8d; font-weight: bold; } QStatusBar { color: #bdc3c7; } QStatusBar QPushButton { background-color: transparent; border: 1px solid #7f8c8d; padding: 2px 8px; font-size: 8pt; } QStatusBar QPushButton:hover { background-color: #34495e; } QSplitter::handle { background-color: #7f8c8d; } QSplitter::handle:horizontal { width: 2px; } )";
const QString MODRINTH_API_BASE = "https://api.modrinth.com/v2";
const QString GITHUB_URL = "https://github.com/helloworldx64/CraftPacker";
const QString PAYPAL_URL = "https://www.paypal.com/donate/?business=4UZWFGSW6C478&no_recurring=0&item_name=Donate+to+helloworldx64¤cy_code=USD";

RateLimiter::RateLimiter(int calls_per_minute) : min_interval(60000 / calls_per_minute) { last_call_time = std::chrono::steady_clock::now() - min_interval; }
void RateLimiter::wait() { QMutexLocker locker(&mutex); auto now = std::chrono::steady_clock::now(); if (auto elapsed = now - last_call_time; elapsed < min_interval) { QThread::msleep(std::chrono::duration_cast<std::chrono::milliseconds>(min_interval - elapsed).count()); } last_call_time = std::chrono::steady_clock::now(); }

DownloadWorker::DownloadWorker(ModInfo mod, QString dir) : modInfo(mod), downloadDir(dir) {}
DownloadWorker::~DownloadWorker() = default;
void DownloadWorker::process() { QString iid = modInfo.isDependency ? modInfo.projectId : modInfo.originalQuery; QNetworkAccessManager manager; QEventLoop loop; connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit); QNetworkRequest request(modInfo.downloadUrl); QNetworkReply *reply = manager.get(request); if (!reply) { emit finished(iid, "Network Error"); return; } QString filePath = downloadDir + "/" + modInfo.filename; QFile file(filePath); if (!file.open(QIODevice::WriteOnly)) { emit finished(iid, "File Error"); reply->abort(); return; } connect(reply, &QNetworkReply::downloadProgress, this, [&](qint64 r, qint64 t){ emit progress(iid, r, t); }); connect(reply, &QNetworkReply::readyRead, [&](){ if(file.isOpen()) { file.write(reply->readAll()); } }); connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit); loop.exec(); file.close(); QString errorString = (reply->error() != QNetworkReply::NoError) ? reply->errorString() : ""; if (!errorString.isEmpty()) { QFile::remove(filePath); } reply->deleteLater(); emit finished(iid, errorString); }

ImageSearchWorker::ImageSearchWorker(QString modName, QString projectId) : m_modName(modName), m_projectId(projectId) {}
void ImageSearchWorker::process() { QNetworkAccessManager manager; QEventLoop loop; QString query = m_modName + " mod minecraft icon"; QUrl url("https://duckduckgo.com/i.js"); QUrlQuery urlQuery; urlQuery.addQueryItem("l", "us-en"); urlQuery.addQueryItem("o", "json"); urlQuery.addQueryItem("q", query); url.setQuery(urlQuery); QNetworkRequest request(url); request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0"); QNetworkReply *reply = manager.get(request); connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit); loop.exec(); if (reply->error() == QNetworkReply::NoError) { QJsonDocument doc = QJsonDocument::fromJson(reply->readAll()); QJsonArray results = doc.object()["results"].toArray(); if (!results.isEmpty()) { QString imageUrl = results[0].toObject()["image"].toString(); QNetworkReply *imageReply = manager.get(QNetworkRequest(QUrl(imageUrl))); connect(imageReply, &QNetworkReply::finished, &loop, &QEventLoop::quit); loop.exec(); if (imageReply->error() == QNetworkReply::NoError) { QPixmap pixmap; pixmap.loadFromData(imageReply->readAll()); emit imageFound(m_projectId, pixmap); } imageReply->deleteLater(); } else { emit imageFound(m_projectId, QPixmap()); } } else { emit imageFound(m_projectId, QPixmap()); } reply->deleteLater(); }

CraftPacker::CraftPacker(QWidget *parent) : QMainWindow(parent), rateLimiter(280) {
    settings = new QSettings(this);
    setupUi();
    applySettings();
    qRegisterMetaType<ModInfo>();
    qRegisterMetaType<QList<ModInfo>>();
    qRegisterMetaType<qint64>();
    connect(this, &CraftPacker::onModFound, this, &CraftPacker::onModFound, Qt::QueuedConnection);
    connect(this, &CraftPacker::onModNotFound, this, &CraftPacker::onModNotFound, Qt::QueuedConnection);
    connect(this, &CraftPacker::onSearchFinished, this, &CraftPacker::onSearchFinished, Qt::QueuedConnection);
    connect(this, &CraftPacker::updateStatusBar, this, &CraftPacker::updateStatusBar, Qt::QueuedConnection);
    connect(this, &CraftPacker::onAllDownloadsFinished, this, &CraftPacker::onAllDownloadsFinished, Qt::QueuedConnection);
    connect(this, &CraftPacker::onDependencyResolutionFinished, this, &CraftPacker::onDependencyResolutionFinished, Qt::QueuedConnection);
    connect(this, &CraftPacker::onImageFound, this, &CraftPacker::onImageFound, Qt::QueuedConnection);
    auto *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(400);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::InQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    profilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(profilePath);
    loadProfileList();
    setAcceptDrops(true);
}
CraftPacker::~CraftPacker() = default;

void CraftPacker::setupUi() {
    setWindowTitle("CraftPacker");
    setMinimumSize(1200, 750);
    QMenuBar* menuBar = new QMenuBar();
    QMenu* fileMenu = menuBar->addMenu("File");
    QAction* settingsAction = fileMenu->addAction("Settings...");
    connect(settingsAction, &QAction::triggered, this, &CraftPacker::openSettingsDialog);
    QAction* exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    setMenuBar(menuBar);
    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    QWidget* leftContainer = new QWidget();
    QVBoxLayout *leftColumnLayout = new QVBoxLayout(leftContainer);
    QGroupBox *settingsGroup = new QGroupBox("Settings");
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);
    settingsLayout->addWidget(new QLabel("MC Version:"), 0, 0);
    mcVersionEntry = new QLineEdit("1.20.1");
    settingsLayout->addWidget(mcVersionEntry, 0, 1);
    settingsLayout->addWidget(new QLabel("Loader:"), 0, 2);
    loaderComboBox = new QComboBox;
    loaderComboBox->addItems({"fabric", "forge", "neoforge", "quilt"});
    settingsLayout->addWidget(loaderComboBox, 0, 3);
    settingsLayout->setColumnStretch(1, 1);
    settingsLayout->addWidget(new QLabel("Download To:"), 1, 0);
    dirEntry = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/CraftPacker_Downloads");
    settingsLayout->addWidget(dirEntry, 1, 1, 1, 3);
    QPushButton *browseButton = new QPushButton("Browse...");
    settingsLayout->addWidget(browseButton, 1, 4);
    QGroupBox *inputGroup = new QGroupBox("Mod List");
    QVBoxLayout *inputLayout = new QVBoxLayout(inputGroup);
    modlistInput = new QTextEdit;
    QPushButton *importButton = new QPushButton("Import from Folder...");
    inputLayout->addWidget(modlistInput);
    inputLayout->addWidget(importButton);
    QGroupBox *profileGroup = new QGroupBox("Mod Profiles");
    QVBoxLayout *profileLayout = new QVBoxLayout(profileGroup);
    profileListWidget = new QListWidget();
    QHBoxLayout *profileButtonsLayout = new QHBoxLayout();
    loadProfileButton = new QPushButton("Load");
    saveProfileButton = new QPushButton("Save");
    deleteProfileButton = new QPushButton("Delete");
    profileButtonsLayout->addWidget(loadProfileButton);
    profileButtonsLayout->addWidget(saveProfileButton);
    profileButtonsLayout->addWidget(deleteProfileButton);
    profileLayout->addWidget(profileListWidget);
    profileLayout->addLayout(profileButtonsLayout);
    leftColumnLayout->addWidget(settingsGroup);
    leftColumnLayout->addWidget(inputGroup);
    leftColumnLayout->addWidget(profileGroup);
    QWidget* rightContainer = new QWidget();
    QVBoxLayout *rightColumnLayout = new QVBoxLayout(rightContainer);
    QGroupBox *resultsGroup = new QGroupBox("Results");
    QHBoxLayout* resultsLayout = new QHBoxLayout(resultsGroup);
    QGroupBox *foundGroup = new QGroupBox("Available Mods");
    QVBoxLayout *foundLayout = new QVBoxLayout(foundGroup);
    foundTree = new QTreeWidget;
    foundTree->setColumnCount(3);
    foundTree->setHeaderLabels({"Official Mod Name", "Status", "Progress"});
    foundTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    foundTree->setAlternatingRowColors(true);
    foundTree->setContextMenuPolicy(Qt::CustomContextMenu); // Enable context menu
    foundLayout->addWidget(foundTree);
    QGroupBox *notFoundGroup = new QGroupBox("Not Found");
    notFoundGroup->setFixedWidth(250);
    QVBoxLayout *notFoundLayout = new QVBoxLayout(notFoundGroup);
    notFoundTree = new QTreeWidget;
    notFoundTree->setColumnCount(2);
    notFoundTree->setHeaderLabels({"Name", "Note"});
    notFoundLayout->addWidget(notFoundTree);
    notFoundTree->setContextMenuPolicy(Qt::CustomContextMenu);
    resultsLayout->addWidget(foundGroup);
    resultsLayout->addWidget(notFoundGroup);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    searchButton = new QPushButton("Search Mods");
    researchButton = new QPushButton("Re-Search Not Found");
    downloadSelectedButton = new QPushButton("Download Selected");
    downloadAllButton = new QPushButton("Download All Available");
    buttonLayout->addWidget(searchButton);
    buttonLayout->addWidget(researchButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(downloadSelectedButton);
    buttonLayout->addWidget(downloadAllButton);
    rightColumnLayout->addWidget(resultsGroup);
    rightColumnLayout->addLayout(buttonLayout);
    modInfoPanel = new QWidget();
    modInfoPanel->setFixedWidth(300);
    QVBoxLayout* modInfoLayout = new QVBoxLayout(modInfoPanel);
    QGroupBox* modInfoGroup = new QGroupBox("Mod Details");
    QVBoxLayout* modInfoGroupL = new QVBoxLayout(modInfoGroup);
    modIconLabel = new QLabel("Select a mod to see details");
    modIconLabel->setAlignment(Qt::AlignCenter);
    modIconLabel->setFixedSize(128, 128);
    modIconLabel->setStyleSheet("border: 1px solid #7f8c8d; border-radius: 5px;");
    modTitleLabel = new QLabel();
    modTitleLabel->setWordWrap(true);
    modTitleLabel->setStyleSheet("font-size: 12pt; font-weight: bold;");
    modAuthorLabel = new QLabel();
    modSummaryText = new QTextEdit();
    modSummaryText->setReadOnly(true);
    modInfoGroupL->addWidget(modIconLabel, 0, Qt::AlignCenter);
    modInfoGroupL->addWidget(modTitleLabel);
    modInfoGroupL->addWidget(modAuthorLabel);
    modInfoGroupL->addWidget(modSummaryText);
    modInfoLayout->addWidget(modInfoGroup);
    mainSplitter->addWidget(leftContainer);
    mainSplitter->addWidget(rightContainer);
    mainSplitter->addWidget(modInfoPanel);
    mainSplitter->setStretchFactor(1, 1);
    QHBoxLayout *centralLayout = new QHBoxLayout(centralWidget);
    centralLayout->addWidget(mainSplitter);
    QStatusBar *statusBar = new QStatusBar;
    setStatusBar(statusBar);
    statusLabel = new QLabel("Ready");
    statusBar->addWidget(statusLabel);
    QPushButton *githubButton = new QPushButton("GitHub");
    QPushButton *paypalButton = new QPushButton("Donate");
    statusBar->addPermanentWidget(paypalButton);
    statusBar->addPermanentWidget(githubButton);
    actionButtons = {searchButton, researchButton, downloadSelectedButton, downloadAllButton, loadProfileButton, saveProfileButton, deleteProfileButton};
    connect(browseButton, &QPushButton::clicked, this, &CraftPacker::browseDirectory);
    connect(importButton, &QPushButton::clicked, this, &CraftPacker::importFromFolder);
    connect(searchButton, &QPushButton::clicked, this, &CraftPacker::startSearch);
    connect(researchButton, &QPushButton::clicked, this, &CraftPacker::startReSearch);
    connect(downloadSelectedButton, &QPushButton::clicked, this, &CraftPacker::startDownloadSelected);
    connect(downloadAllButton, &QPushButton::clicked, this, &CraftPacker::startDownloadAll);
    connect(githubButton, &QPushButton::clicked, this, &CraftPacker::openGitHub);
    connect(paypalButton, &QPushButton::clicked, this, &CraftPacker::openPayPal);
    connect(loadProfileButton, &QPushButton::clicked, this, &CraftPacker::loadProfile);
    connect(saveProfileButton, &QPushButton::clicked, this, &CraftPacker::saveProfile);
    connect(deleteProfileButton, &QPushButton::clicked, this, &CraftPacker::deleteProfile);
    connect(profileListWidget, &QListWidget::itemDoubleClicked, this, &CraftPacker::loadProfile);
    connect(foundTree, &QTreeWidget::currentItemChanged, this, &CraftPacker::updateModInfoPanel);
    connect(foundTree, &QTreeWidget::customContextMenuRequested, this, &CraftPacker::showFoundContextMenu);
    connect(notFoundTree, &QTreeWidget::customContextMenuRequested, this, &CraftPacker::showNotFoundContextMenu);
}

void CraftPacker::dragEnterEvent(QDragEnterEvent *event) { if (event->mimeData()->hasUrls()) { event->acceptProposedAction(); } }
void CraftPacker::dropEvent(QDropEvent *event) { const QMimeData* mimeData = event->mimeData(); if (mimeData->hasUrls()) { QUrl url = mimeData->urls().first(); if (url.isLocalFile() && url.toLocalFile().endsWith(".txt")) { QFile file(url.toLocalFile()); if (file.open(QIODevice::ReadOnly | QIODevice::Text)) { modlistInput->setText(file.readAll()); updateStatusBar("Loaded from " + QFileInfo(file).fileName()); } } } }
void CraftPacker::setButtonsEnabled(bool enabled) { for (auto* b : actionButtons) { if(b) b->setEnabled(enabled); } }
void CraftPacker::updateStatusBar(const QString &text) { statusLabel->setText(text); }
void CraftPacker::browseDirectory() { QString d = QFileDialog::getExistingDirectory(this, "", dirEntry->text()); if (!d.isEmpty()) { dirEntry->setText(d); } }
void CraftPacker::importFromFolder() { QString p = QFileDialog::getExistingDirectory(this, ""); if (p.isEmpty()) return; QDirIterator it(p, {"*.jar"}, QDir::Files, QDirIterator::Subdirectories); QSet<QString> names; while(it.hasNext()){ it.next(); QString baseName = it.fileInfo().baseName(); QString cleanName = sanitizeModName(baseName); if (!cleanName.isEmpty()) { names.insert(cleanName); } } if (!names.isEmpty()) { modlistInput->setText(QStringList(names.values()).join('\n')); } }
void CraftPacker::startSearch() {
    if (modlistInput->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Empty List", "The mod list is empty. Please enter some mod names to search for.");
        return;
    }
    QStringList l = modlistInput->toPlainText().split('\n', Qt::SkipEmptyParts);
    foundTree->clear(); notFoundTree->clear(); results.clear(); treeItems.clear(); allFoundOrDependencyProjects.clear();
    startModSearch(l);
}
void CraftPacker::startReSearch() {
    if (notFoundTree->topLevelItemCount() == 0) {
        QMessageBox::warning(this, "Empty List", "There are no mods in the 'Not Found' list to re-search.");
        return;
    }
    QStringList l; for (int i = 0; i < notFoundTree->topLevelItemCount(); ++i) { l.append(notFoundTree->topLevelItem(i)->text(0)); }
    notFoundTree->clear();
    startModSearch(l);
}
void CraftPacker::onSearchFinished() { updateStatusBar(QString("Search complete. Found %1 of %2 mods.").arg(results.size()).arg(results.size() + notFoundTree->topLevelItemCount())); setButtonsEnabled(true); runJumpAnimation(downloadAllButton); }
void CraftPacker::onModFound(const ModInfo& modInfo, const QString& status, const QString& tag) { QMutexLocker l(&searchMutex); if (allFoundOrDependencyProjects.contains(modInfo.projectId)) { if(searchCounter.fetchAndAddRelaxed(-1) - 1 <= 0) { onSearchFinished(); } return; } allFoundOrDependencyProjects.insert(modInfo.projectId); results.insert(modInfo.originalQuery, modInfo); QTreeWidgetItem* i = new QTreeWidgetItem(foundTree); i->setText(0, modInfo.name); i->setText(1, status); i->setData(0, Qt::UserRole, modInfo.originalQuery); if (tag == "found") i->setForeground(1, QColor("#27ae60")); else if (tag == "fallback") i->setForeground(1, QColor("#f39c12")); else if(tag == "web_fallback") i->setForeground(1, QColor("#1abc9c")); else if (tag == "dependency") { i->setForeground(1, QColor("#8e44ad")); runCompletionAnimation(i); } treeItems.insert(modInfo.originalQuery, i); if (searchCounter.fetchAndAddRelaxed(-1) - 1 <= 0) { onSearchFinished(); } }
void CraftPacker::onModNotFound(const QString& n) { QMutexLocker l(&searchMutex); new QTreeWidgetItem(notFoundTree, {n, "(Check CurseForge?)"}); if (searchCounter.fetchAndAddRelaxed(-1) - 1 <= 0) { onSearchFinished(); } }
void CraftPacker::findOneMod(QString name, QString loader, QString version) { emit updateStatusBar("Searching for: " + name); QString cleanName = sanitizeModName(name); QString spacedName = splitCamelCase(cleanName); QNetworkAccessManager manager; QEventLoop loop; auto performRequest = [&](const QUrl& url) -> std::optional<QJsonDocument> { rateLimiter.wait(); QNetworkReply *reply = manager.get(QNetworkRequest(url)); connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit); loop.exec(); if (reply->error() == QNetworkReply::NoError) { auto data = reply->readAll(); reply->deleteLater(); return QJsonDocument::fromJson(data); } reply->deleteLater(); return std::nullopt; }; QUrlQuery query; query.addQueryItem("query", spacedName); query.addQueryItem("limit", "5"); query.addQueryItem("facets", R"([["project_type:mod"]])"); QUrl url(MODRINTH_API_BASE + "/search"); url.setQuery(query); if (auto doc = performRequest(url)) { if (!doc->object().isEmpty() && doc->object().contains("hits")) { for (const auto& hitVal : doc->object()["hits"].toArray()) { QString projectId = hitVal.toObject()["project_id"].toString(); if (auto modInfo = getModInfo(projectId, loader, version)) { auto finalInfo = modInfo.value(); finalInfo.originalQuery = name; emit onModFound(finalInfo, "Available (API)", "found"); return; } } } } QString slug = cleanName.toLower().replace(QRegularExpression(R"(\s)"), "-"); if (auto modInfo = getModInfo(slug, loader, version)) { auto finalInfo = modInfo.value(); finalInfo.originalQuery = name; emit onModFound(finalInfo, "Available (Slug)", "fallback"); return; } emit onModNotFound(name); }
void CraftPacker::startModSearch(const QStringList &modNames) { setButtonsEnabled(false); updateStatusBar("Searching..."); searchCounter = modNames.size(); QString loader = loaderComboBox->currentText(); QString version = mcVersionEntry->text(); for (const auto& name : modNames) { QThreadPool::globalInstance()->start([this, name, loader, version]() { findOneMod(name.trimmed(), loader, version); }); } }
void CraftPacker::startDownloadSelected() {
    auto sel = foundTree->selectedItems();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select one or more mods from the 'Available Mods' list to download.");
        return;
    }
    QStringList ids; for (auto* i : sel) { ids.append(i->data(0, Qt::UserRole).toString()); }
    startDownload(ids);
}
void CraftPacker::startDownloadAll() {
    if (results.isEmpty()) {
        QMessageBox::warning(this, "No Mods Found", "There are no available mods to download. Please search for mods first.");
        return;
    }
    startDownload(results.keys());
}
void CraftPacker::startDownload(const QList<QString>& itemIds) { setButtonsEnabled(false); activeDownloads = 0; QList<ModInfo> initialMods; for (const auto& id : itemIds) { if (results.contains(id)) { if(QFile::exists(dirEntry->text() + "/" + results[id].filename)) continue; initialMods.append(results[id]); } } if (initialMods.isEmpty()) { updateStatusBar("All selected mods are already downloaded."); setButtonsEnabled(true); return; } updateStatusBar("Resolving dependencies..."); QThreadPool::globalInstance()->start([this, initialMods]() { resolveDependenciesAndDownload(initialMods); }); }
void CraftPacker::resolveDependenciesAndDownload(QList<ModInfo> initialMods) { QList<ModInfo> dq; QSet<QString> sp; QList<ModInfo> tr = initialMods; while (!tr.isEmpty()) { ModInfo c = tr.takeFirst(); if (sp.contains(c.projectId)) continue; sp.insert(c.projectId); dq.prepend(c); for (const QJsonObject& d : c.dependencies) { if (d["dependency_type"].toString() == "required") { emit updateStatusBar("Resolving dependency: " + d["project_id"].toString()); if (auto diOpt = getModInfo(d["project_id"].toString(), loaderComboBox->currentText(), mcVersionEntry->text())) { auto di = diOpt.value(); di.isDependency = true; tr.append(di); } } } } emit onDependencyResolutionFinished(dq); }
void CraftPacker::onDependencyResolutionFinished(const QList<ModInfo>& dq) { QDir().mkpath(dirEntry->text()); int c=0; for(const auto& m:dq){ QString filePath = dirEntry->text() + "/" + m.filename; if(QFile::exists(filePath)) continue; c++;} activeDownloads = c; if(c==0){ onAllDownloadsFinished(); return;} for(const auto& m:dq){ QString filePath = dirEntry->text() + "/" + m.filename; if(QFile::exists(filePath)) continue; if(m.isDependency){emit onModFound(m,"Dependency","dependency");} else if(treeItems.contains(m.originalQuery)){runSwooshAnimation(treeItems[m.originalQuery]);} QThread* t=new QThread; DownloadWorker* w=new DownloadWorker(m,dirEntry->text()); w->moveToThread(t); connect(t,&QThread::started,w,&DownloadWorker::process); connect(w,&DownloadWorker::progress,this,&CraftPacker::onDownloadProgress); connect(w,&DownloadWorker::finished,this,&CraftPacker::onDownloadFinished); connect(w,&DownloadWorker::finished,t,&QThread::quit); connect(w,&DownloadWorker::finished,w,&QObject::deleteLater); connect(t,&QThread::finished,t,&QObject::deleteLater); t->start();} }
void CraftPacker::onDownloadProgress(const QString& iid, qint64 r, qint64 t) { for(auto it=results.constBegin();it!=results.constEnd();++it){ if(it.value().projectId == iid || it.key() == iid){ if(treeItems.contains(it.key())&&t>0){if(auto* i=treeItems[it.key()]){int p=(int)(((double)r/t)*100.0); i->setText(2,QString::number(p)+"%");}} return; } } }
void CraftPacker::onDownloadFinished(const QString& iid, const QString& e) { for(auto it=results.constBegin();it!=results.constEnd();++it){ if(it.value().projectId == iid || it.key() == iid){ if (treeItems.contains(it.key())) { if (auto* i = treeItems[it.key()]) { if (e.isEmpty()) { i->setText(1, "Complete"); i->setText(2, "100%"); i->setForeground(1, QColor("#2ecc71")); runCompletionAnimation(i); } else { i->setText(1, "Error"); i->setForeground(1, QColor("#e74c3c")); } } } break; } } if (activeDownloads.fetchAndAddRelaxed(-1) - 1 == 0) { onAllDownloadsFinished(); } }
void CraftPacker::runCompletionAnimation(QTreeWidgetItem *item) { if (!item || isMinimized()) return; auto* a = new QVariantAnimation(this); a->setDuration(800); a->setStartValue(0.0); a->setEndValue(1.0); QColor fc("#27ae60"), bc("#34495e"); connect(a, &QVariantAnimation::valueChanged, this, [item, fc, bc](const QVariant& v){ if (!item) return; qreal p = v.toReal(); QColor nc; nc.setRedF(bc.redF() + (fc.redF() - bc.redF()) * (1.0 - p)); nc.setGreenF(bc.greenF() + (fc.greenF() - bc.greenF()) * (1.0 - p)); nc.setBlueF(bc.blueF() + (fc.blueF() - bc.blueF()) * (1.0 - p)); for(int i = 0; i < item->columnCount(); ++i) { item->setBackground(i, nc); } }); connect(a, &QVariantAnimation::finished, this, [item, bc](){ if (!item) return; for(int i = 0; i < item->columnCount(); ++i) { item->setBackground(i, bc); } }); a->start(QAbstractAnimation::DeleteWhenStopped); }
void CraftPacker::runJumpAnimation(QWidget* widget) { if(!widget) return; QPoint startPos = widget->pos(); QPropertyAnimation* anim = new QPropertyAnimation(widget, "pos", this); anim->setDuration(400); anim->setStartValue(startPos); anim->setKeyValueAt(0.5, startPos - QPoint(0, 10)); anim->setEndValue(startPos); anim->setEasingCurve(QEasingCurve::OutBounce); anim->start(QAbstractAnimation::DeleteWhenStopped); }
void CraftPacker::runSwooshAnimation(QTreeWidgetItem* item) { if(!item) return; runCompletionAnimation(item); }
void CraftPacker::onAllDownloadsFinished() { updateStatusBar("All downloads completed."); setButtonsEnabled(true); }
void CraftPacker::openGitHub() { QDesktopServices::openUrl(QUrl(GITHUB_URL)); }
void CraftPacker::openPayPal() { QDesktopServices::openUrl(QUrl(PAYPAL_URL)); }
void CraftPacker::loadProfileList() { profileListWidget->clear(); QDir d(profilePath); d.setNameFilters({"*.txt"}); for (const auto& fi : d.entryInfoList(QDir::Files)) { profileListWidget->addItem(fi.baseName()); } }
void CraftPacker::saveProfile() { bool ok; QString t = QInputDialog::getText(this, "Save Profile", "Profile Name:", QLineEdit::Normal, "", &ok); if (ok && !t.isEmpty()) { QFile f(profilePath + "/" + t + ".txt"); if (f.open(QIODevice::WriteOnly | QIODevice::Text)) { QTextStream o(&f); o << modlistInput->toPlainText(); f.close(); loadProfileList(); } } }
void CraftPacker::loadProfile() { auto sel = profileListWidget->selectedItems(); if (sel.isEmpty()) return; QString n = sel.first()->text(); QFile f(profilePath + "/" + n + ".txt"); if (f.open(QIODevice::ReadOnly | QIODevice::Text)) { QTextStream i(&f); modlistInput->setText(i.readAll()); updateStatusBar("Profile '" + n + "' loaded."); } }
void CraftPacker::deleteProfile() { auto sel = profileListWidget->selectedItems(); if (sel.isEmpty()) return; QString n = sel.first()->text(); if (QMessageBox::question(this, "Confirm Delete", "Are you sure you want to delete profile '" + n + "'?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) { if (QFile::remove(profilePath + "/" + n + ".txt")) { loadProfileList(); updateStatusBar("Profile '" + n + "' deleted."); } } }
void CraftPacker::openSettingsDialog() { QDialog settingsDialog(this); settingsDialog.setWindowTitle("Settings"); QFormLayout form(&settingsDialog); QSpinBox* threadCountSpinBox = new QSpinBox(&settingsDialog); threadCountSpinBox->setRange(1, QThread::idealThreadCount()); threadCountSpinBox->setValue(settings->value("maxThreads", QThread::idealThreadCount()).toInt()); form.addRow("Max Concurrent Downloads/Searches:", threadCountSpinBox); QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &settingsDialog); form.addRow(&buttonBox); connect(&buttonBox, &QDialogButtonBox::accepted, &settingsDialog, &QDialog::accept); connect(&buttonBox, &QDialogButtonBox::rejected, &settingsDialog, &QDialog::reject); if (settingsDialog.exec() == QDialog::Accepted) { settings->setValue("maxThreads", threadCountSpinBox->value()); applySettings(); } }
void CraftPacker::applySettings() { QThreadPool::globalInstance()->setMaxThreadCount(settings->value("maxThreads", QThread::idealThreadCount()).toInt()); }
void CraftPacker::updateModInfoPanel(QTreeWidgetItem *current, QTreeWidgetItem *) { if (!current) return; QString modKey = current->data(0, Qt::UserRole).toString(); if (!results.contains(modKey)) return; ModInfo mod = results[modKey]; modTitleLabel->setText(mod.name); modIconLabel->setText("Loading..."); QThreadPool::globalInstance()->start([this, mod]() { rateLimiter.wait(); QNetworkAccessManager manager; QEventLoop loop; QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(MODRINTH_API_BASE + "/project/" + mod.projectId))); connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit); loop.exec(); if (reply->error() == QNetworkReply::NoError) { QJsonObject projectDetails = QJsonDocument::fromJson(reply->readAll()).object(); QString author = projectDetails["author"].toString(); QString summary = projectDetails["description"].toString(); QString iconUrl = projectDetails["icon_url"].toString(); QMetaObject::invokeMethod(this, [this, author, summary](){ modAuthorLabel->setText("by " + author); modSummaryText->setText(summary);}, Qt::QueuedConnection); if (!iconUrl.isNull() && !iconUrl.isEmpty()) { QNetworkReply* iconReply = manager.get(QNetworkRequest(QUrl(iconUrl))); connect(iconReply, &QNetworkReply::finished, &loop, &QEventLoop::quit); loop.exec(); if(iconReply->error() == QNetworkReply::NoError) { QPixmap pixmap; pixmap.loadFromData(iconReply->readAll()); onImageFound(mod.projectId, pixmap); } else { onImageFound(mod.projectId, QPixmap()); } iconReply->deleteLater(); } else { QThread* imageWorkerThread = new QThread; ImageSearchWorker* worker = new ImageSearchWorker(mod.name, mod.projectId); worker->moveToThread(imageWorkerThread); connect(imageWorkerThread, &QThread::started, worker, &ImageSearchWorker::process); connect(worker, &ImageSearchWorker::imageFound, this, &CraftPacker::onImageFound, Qt::QueuedConnection); connect(worker, &ImageSearchWorker::imageFound, imageWorkerThread, &QThread::quit); connect(imageWorkerThread, &QThread::finished, worker, &QObject::deleteLater); connect(imageWorkerThread, &QThread::finished, imageWorkerThread, &QObject::deleteLater); imageWorkerThread->start(); } } reply->deleteLater(); }); }
void CraftPacker::onImageFound(const QString& projectId, const QPixmap& pixmap) { auto* currentItem = foundTree->currentItem(); if(!currentItem) return; QString key = currentItem->data(0, Qt::UserRole).toString(); if(results.contains(key) && results[key].projectId == projectId) { if (!pixmap.isNull()) { modIconLabel->setPixmap(pixmap.scaled(modIconLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)); } else { modIconLabel->setText("No Icon Found"); } } }
void CraftPacker::showFoundContextMenu(const QPoint &pos) {
    QTreeWidgetItem* item = foundTree->itemAt(pos);
    if (!item) return;

    QString modKey = item->data(0, Qt::UserRole).toString();
    if (!results.contains(modKey)) return;

    ModInfo mod = results[modKey];
    QMenu contextMenu(this);
    contextMenu.addAction("Open on Modrinth", [mod]() {
        QDesktopServices::openUrl(QUrl("https://modrinth.com/mod/" + mod.projectId));
    });
    contextMenu.exec(foundTree->mapToGlobal(pos));
}
void CraftPacker::showNotFoundContextMenu(const QPoint &pos) { QMenu contextMenu(this); QTreeWidgetItem* item = notFoundTree->itemAt(pos); if (item) { contextMenu.addAction("Copy Name", [item](){ QApplication::clipboard()->setText(item->text(0)); }); contextMenu.addAction("Search on CurseForge", [this, item](){ QUrlQuery query; query.addQueryItem("q", "site:curseforge.com/minecraft/mc-mods " + item->text(0)); QUrl url("https://google.com/search"); url.setQuery(query); QDesktopServices::openUrl(url); }); } contextMenu.addSeparator(); contextMenu.addAction("Copy All Names", [this](){ QStringList names; for(int i=0; i < notFoundTree->topLevelItemCount(); ++i) { names.append(notFoundTree->topLevelItem(i)->text(0)); } QApplication::clipboard()->setText(names.join('\n')); }); contextMenu.exec(notFoundTree->mapToGlobal(pos)); }
QString CraftPacker::sanitizeModName(const QString& input) { QString result = input; result = result.remove(QRegularExpression(R"(\b(fabric|forge|quilt|neoforge|\+1|mod)\b)", QRegularExpression::CaseInsensitiveOption)); result = result.remove(QRegularExpression(R"([\[\]\(\)])")); result = result.remove(QRegularExpression(R"((?i)[-_]?(fabric|forge|quilt|neoforge)?[-_]?\d+(\.\d+)*([-_].*)?$)")); result = result.replace(QRegularExpression("[-_]"), " ").trimmed(); return result.simplified(); }
QString CraftPacker::splitCamelCase(const QString& input) { QString temp = input; QRegularExpression re("(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])"); return temp.replace(re, " "); }

std::optional<ModInfo> CraftPacker::getModInfo(const QString& projectIdOrSlug, const QString& loader, const QString& gameVersion) {
    rateLimiter.wait();
    QNetworkAccessManager manager;
    QEventLoop loop;
    auto performRequest = [&](const QUrl& url) -> std::optional<QByteArray> {
        QNetworkReply *reply = manager.get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) {
            auto data = reply->readAll();
            reply->deleteLater();
            return data;
        }
        reply->deleteLater();
        return std::nullopt;
    };
    auto projDataOpt = performRequest(QUrl(MODRINTH_API_BASE + "/project/" + projectIdOrSlug));
    if (!projDataOpt) return std::nullopt;
    QJsonDocument doc = QJsonDocument::fromJson(projDataOpt.value());
    if (doc.isNull() || !doc.isObject()) return std::nullopt;
    QJsonObject projObj = doc.object();
    if (projObj.isEmpty()) return std::nullopt;
    QUrlQuery query;
    query.addQueryItem("loaders", QJsonDocument(QJsonArray({loader})).toJson(QJsonDocument::Compact));
    query.addQueryItem("game_versions", QJsonDocument(QJsonArray({gameVersion})).toJson(QJsonDocument::Compact));
    QUrl versionUrl(MODRINTH_API_BASE + "/project/" + projObj.value("slug").toString() + "/version");
    versionUrl.setQuery(query);
    auto versionsDataOpt = performRequest(versionUrl);
    if (!versionsDataOpt) return std::nullopt;
    QJsonDocument versionsDoc = QJsonDocument::fromJson(versionsDataOpt.value());
    if (versionsDoc.isNull() || !versionsDoc.isArray()) return std::nullopt;
    QJsonArray versions = versionsDoc.array();
    for (const QString& type : {"release", "beta", "alpha"}) {
        for (const auto& verVal : versions) {
            QJsonObject verObj = verVal.toObject();
            if (verObj["version_type"].toString() == type) {
                QJsonArray files = verObj["files"].toArray();
                if (files.isEmpty()) continue;
                QJsonObject fileObj = files[0].toObject();
                ModInfo info;
                info.name = projObj["title"].toString();
                info.projectId = projObj["id"].toString();
                info.versionId = verObj["id"].toString();
                info.downloadUrl = fileObj["url"].toString();
                info.filename = fileObj["filename"].toString();
                info.versionType = verObj["version_type"].toString();
                info.dependencies.clear();
                for (const auto& depVal : verObj["dependencies"].toArray()) {
                    info.dependencies.append(depVal.toObject());
                }
                return info;
            }
        }
    }
    return std::nullopt;
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("CraftPacker");
    QCoreApplication::setApplicationName("CraftPacker");
    QApplication app(argc, argv);
    app.setStyleSheet(STYLESHEET);
    CraftPacker window;
    window.show();
    return app.exec();
}
