#ifndef CRAFTPACKER_H
#define CRAFTPACKER_H

#include <QObject>
#include <QMainWindow>
#include <QJsonObject>
#include <QTreeWidget>
#include <QListWidget>
#include <QMutex>
#include <QAtomicInt>
#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QComboBox;
class QTextEdit;
class QPushButton;
class QLabel;
class QPropertyAnimation;
class QSplitter;
class QMenu;
class QMenuBar;
class QSettings;
QT_END_NAMESPACE

struct ModInfo {
    QString originalQuery;
    QString name;
    QString projectId;
    QString versionId;
    QString downloadUrl;
    QString filename;
    QString versionType;
    QList<QJsonObject> dependencies;
    bool isDependency = false;
    bool updateAvailable = false;
};
Q_DECLARE_METATYPE(ModInfo);

class RateLimiter {
public:
    RateLimiter(int calls_per_minute);
    void wait();
private:
    std::chrono::milliseconds min_interval;
    QMutex mutex;
    std::chrono::steady_clock::time_point last_call_time;
};

class DownloadWorker : public QObject {
    Q_OBJECT
public:
    DownloadWorker(ModInfo mod, QString dir);
    ~DownloadWorker();
public slots:
    void process();
signals:
    void progress(const QString& iid, qint64 bytesReceived, qint64 bytesTotal);
    void finished(const QString& iid, const QString& error);
private:
    ModInfo modInfo;
    QString downloadDir;
};

class ImageSearchWorker : public QObject {
    Q_OBJECT
public:
    ImageSearchWorker(QString modName, QString projectId);
public slots:
    void process();
signals:
    void imageFound(const QString& projectId, const QPixmap& pixmap);
private:
    QString m_modName;
    QString m_projectId;
};

class CraftPacker : public QMainWindow {
    Q_OBJECT

public:
    CraftPacker(QWidget *parent = nullptr);
    ~CraftPacker();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void browseDirectory();
    void importFromFolder();
    void startSearch();
    void startReSearch();
    void startDownloadSelected();
    void startDownloadAll();
    void openGitHub();
    void openPayPal();
    void saveProfile();
    void loadProfile();
    void deleteProfile();
    void openSettingsDialog();
    void updateModInfoPanel(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void onImageFound(const QString& projectId, const QPixmap& pixmap);
    void showNotFoundContextMenu(const QPoint &pos);
    void showFoundContextMenu(const QPoint &pos); // New slot for the found mods list
    void onModFound(const ModInfo& modInfo, const QString& status, const QString& tag);
    void onModNotFound(const QString& modName);
    void onSearchFinished();
    void onDownloadProgress(const QString& iid, qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(const QString& iid, const QString& error);
    void updateStatusBar(const QString& text);
    void onAllDownloadsFinished();
    void onDependencyResolutionFinished(const QList<ModInfo>& downloadQueue);

private:
    void setupUi();
    void setButtonsEnabled(bool enabled);
    void startModSearch(const QStringList& modNames);
    void findOneMod(QString name, QString loader, QString version);
    void startDownload(const QList<QString>& itemIds);
    void resolveDependenciesAndDownload(QList<ModInfo> initialMods);
    void runCompletionAnimation(QTreeWidgetItem* item);
    void runJumpAnimation(QWidget* widget);
    void runSwooshAnimation(QTreeWidgetItem* item);
    void loadProfileList();
    void applySettings();
    QString sanitizeModName(const QString& input);
    QString splitCamelCase(const QString& input);
    std::optional<ModInfo> getModInfo(const QString& projectId, const QString& loader, const QString& gameVersion);

    QLineEdit* mcVersionEntry;
    QComboBox* loaderComboBox;
    QLineEdit* dirEntry;
    QTextEdit* modlistInput;
    QTreeWidget* foundTree;
    QTreeWidget* notFoundTree;
    QPushButton* searchButton, *researchButton, *downloadSelectedButton, *downloadAllButton;
    QLabel* statusLabel;
    QList<QPushButton*> actionButtons;
    QListWidget* profileListWidget;
    QPushButton* loadProfileButton, *saveProfileButton, *deleteProfileButton;
    QSplitter* mainSplitter;
    QWidget* modInfoPanel;
    QLabel* modIconLabel, *modTitleLabel, *modAuthorLabel;
    QTextEdit* modSummaryText;

    QSettings* settings;
    RateLimiter rateLimiter;
    QHash<QString, ModInfo> results;
    QHash<QString, QTreeWidgetItem*> treeItems;
    QAtomicInt activeDownloads;
    QSet<QString> allFoundOrDependencyProjects;
    QAtomicInt searchCounter;
    QMutex searchMutex;
    QString profilePath;
};

#endif // CRAFTPACKER_H
