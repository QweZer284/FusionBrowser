#include "extensions.h"
#include "database.h"
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QThread>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>
#include <QDataStream>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QScrollArea>
#include <QFrame>
#include <QMenu>
#include <QListWidgetItem>
#include <QApplication>
#include <QStandardPaths>

// ── Chrome API stub JS ────────────────────────────────────────────────────────
static const char* CHROME_API_JS = R"JS(
(function(){
  if(window.chrome&&window.chrome.runtime&&window.chrome.runtime.id)return;
  const noop=()=>{};
  const noopCb=(cb)=>{if(typeof cb==='function')cb();};
  window.chrome={
    runtime:{
      id:undefined,
      connect:()=>({postMessage:noop,onMessage:{addListener:noop}}),
      sendMessage:(msg,cb)=>{if(cb)cb(undefined);},
      onMessage:{addListener:noop,removeListener:noop},
      onConnect:{addListener:noop},
      getURL:(p)=>p,
      getManifest:()=>({}),
      lastError:undefined,
    },
    app:{isInstalled:false,getDetails:()=>null,runningState:()=>'cannot_run'},
    webstore:{
      install:(url,ok,fail)=>{
        document.title='__fusion_install__'+(url||'');
        if(ok)ok();
      },
      onInstallStageChanged:{addListener:noop},
      onDownloadProgress:{addListener:noop},
    },
    tabs:{create:noop,query:(_,cb)=>{if(cb)cb([]);},onUpdated:{addListener:noop}},
    storage:{
      local:{get:(_,cb)=>{if(cb)cb({});},set:(_,cb)=>noopCb(cb),
             remove:(_,cb)=>noopCb(cb),clear:(cb)=>noopCb(cb),
             onChanged:{addListener:noop}},
      sync:{get:(_,cb)=>{if(cb)cb({});},set:(_,cb)=>noopCb(cb),
            onChanged:{addListener:noop}},
      session:{get:(_,cb)=>{if(cb)cb({});},set:(_,cb)=>noopCb(cb),
               onChanged:{addListener:noop}},
    },
    i18n:{getMessage:(k)=>k,getUILanguage:()=>'ru'},
    extension:{getURL:(p)=>p},
    permissions:{
      contains:(_,cb)=>{if(cb)cb(false);},
      request:(_,cb)=>{if(cb)cb(false);}
    },
    identity:{getAuthToken:(_,cb)=>{if(cb)cb(undefined);}},
    loadTimes:()=>({requestTime:performance.now()/1e3,navigationType:'Other'}),
    csi:()=>({startE:Date.now(),pageT:Date.now()-performance.timing.navigationStart}),
  };
  try{Object.defineProperty(navigator,'vendor',{get:()=>'Google Inc.',configurable:true});}catch(e){}
  Object.defineProperty(window,'chrome',{value:window.chrome,writable:false,configurable:false});
})();
)JS";

// ── CRX unpack ────────────────────────────────────────────────────────────────
ExtInstallResult ExtensionManager::unpackCrx(const QByteArray& data, const QString& outDir) {
    QByteArray zipData;

    if (data.startsWith("Cr24")) {
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);

        char magic[4];
        stream.readRawData(magic, 4);

        quint32 version;
        stream >> version;

        if (version == 2) {
            quint32 pubLen, sigLen;
            stream >> pubLen >> sigLen;
            qint64 zipStart = 16 + pubLen + sigLen;
            zipData = data.mid(zipStart);
        } else if (version == 3) {
            quint32 headerLen;
            stream >> headerLen;
            qint64 zipStart = 12 + headerLen;
            zipData = data.mid(zipStart);
        } else {
            return {false, {}, {}, "Unsupported CRX version"};
        }
    } else {
        zipData = data; // plain ZIP
    }

    // Write zip to temp file, then extract using system zip support
    // Qt doesn't have built-in zip extraction — use QuaZip if available,
    // otherwise fall back to writing the zip and using a subprocess.
    // We implement a minimal ZIP reader here for portability.

    QDir().mkpath(outDir);

    // Write to temp .zip file
    QString tmpZip = QDir::tempPath() + "/fusion_ext_tmp.zip";
    {
        QFile f(tmpZip);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return {false, {}, {}, "Cannot write temp zip"};
        f.write(zipData);
    }

    // Use system unzip on Windows via PowerShell
#ifdef Q_OS_WIN
    int ret = QProcess::execute("powershell",
        {"-Command",
         QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(tmpZip).arg(outDir)});
    QFile::remove(tmpZip);
    if (ret != 0)
        return {false, {}, {}, "Expand-Archive failed"};
#else
    int ret = QProcess::execute("unzip", {"-o", tmpZip, "-d", outDir});
    QFile::remove(tmpZip);
    if (ret != 0)
        return {false, {}, {}, "unzip failed"};
#endif

    // Read manifest
    QFile mf(outDir + "/manifest.json");
    if (!mf.open(QIODevice::ReadOnly))
        return {false, {}, {}, "manifest.json not found"};
    auto doc = QJsonDocument::fromJson(mf.readAll());
    if (!doc.isObject())
        return {false, {}, {}, "manifest.json parse error"};

    QString name = doc.object().value("name").toString();
    if (name.isEmpty()) name = QFileInfo(outDir).fileName();

    return {true, outDir, name, {}};
}

QString ExtensionManager::extractExtId(const QString& urlOrId) {
    static const QList<QRegularExpression> patterns = {
        QRegularExpression(R"(webstore/detail/[^/]+/([a-z]{32}))"),
        QRegularExpression(R"(webstore/detail/([a-z]{32}))"),
        QRegularExpression(R"(/extensions/([a-z]{32}))"),
        QRegularExpression(R"(^([a-z]{32})$)"),
    };
    for (const auto& re : patterns) {
        auto m = re.match(urlOrId.trimmed());
        if (m.hasMatch()) return m.captured(1);
    }
    return {};
}

// ── ExtensionManager ──────────────────────────────────────────────────────────
ExtensionManager::ExtensionManager(QWebEngineProfile* profile,
                                   const QString& storageDir,
                                   QObject* parent)
    : QObject(parent), m_profile(profile), m_dir(storageDir)
{
    QDir().mkpath(storageDir);
    injectChromeApi();
}

void ExtensionManager::injectChromeApi() {
    auto* scripts = m_profile->scripts();
    // Remove old if present
    const auto existing = scripts->find("_fusion_chrome_api");
    for (const auto& s : existing) scripts->remove(s);

    QWebEngineScript script;
    script.setName("_fusion_chrome_api");
    script.setSourceCode(CHROME_API_JS);
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(false);
    scripts->insert(script);
}

bool ExtensionManager::loadExt(const QString& path) {
    if (!m_profile->isOffTheRecord()) {
        try {
            m_profile->loadExtension(path);
            return true;
        } catch (...) {}
    }
    return false;
}

ExtInstallResult ExtensionManager::installFromFile(const QString& crxPath) {
    QFile f(crxPath);
    if (!f.open(QIODevice::ReadOnly))
        return {false, {}, {}, "Cannot open file"};
    QByteArray data = f.readAll();

    QString base = QFileInfo(crxPath).baseName().toLower();
    base.replace(QRegularExpression("[^a-z0-9_\\-]"), "_");
    base = base.left(32);

    QString outDir = m_dir + "/" + base;
    QDir(outDir).removeRecursively();

    return unpackCrx(data, outDir);
}

void ExtensionManager::installFromCws(
    const QString& urlOrId,
    std::function<void(const QString&, const QString&)> onDone,
    std::function<void(const QString&)> onFail)
{
    const QString extId = extractExtId(urlOrId);
    if (extId.isEmpty()) {
        onFail("Не удалось извлечь ID расширения из URL.");
        return;
    }

    const QString url = QString(
        "https://clients2.google.com/service/update2/crx"
        "?response=redirect&os=win&arch=x64&nacl_arch=x86-64"
        "&prod=chromiumcrx&prodchannel=stable&prodversion=124.0.0.0"
        "&acceptformat=crx3&x=id%%3D%1%%26uc").arg(extId);

    auto* nam  = new QNetworkAccessManager(this);
    auto* req  = new QNetworkRequest(QUrl(url));
    req->setRawHeader("User-Agent",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36");

    auto* reply = nam->get(*req);
    delete req;

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, extId, onDone, onFail, nam]() mutable {
            reply->deleteLater();
            nam->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                onFail("Ошибка скачивания: " + reply->errorString());
                return;
            }

            const QByteArray data = reply->readAll();
            const QString outDir  = m_dir + "/" + extId;
            QDir(outDir).removeRecursively();

            auto result = unpackCrx(data, outDir);
            if (!result.ok)
                onFail("Не удалось распаковать CRX: " + result.error);
            else
                onDone(result.path, result.name);
        });
}

void ExtensionManager::openPopup(const QString& extDir, QWidget* parent) {
    QFile mf(extDir + "/manifest.json");
    if (!mf.open(QIODevice::ReadOnly)) return;
    auto doc = QJsonDocument::fromJson(mf.readAll());
    if (!doc.isObject()) return;

    auto obj    = doc.object();
    auto action = obj.value("action").toObject();
    if (action.isEmpty()) action = obj.value("browser_action").toObject();
    const QString popup = action.value("default_popup").toString();
    if (popup.isEmpty()) {
        QMessageBox::information(parent, "Fusion",
            "У этого расширения нет popup-окна.");
        return;
    }
    const QString popupPath = extDir + "/" + popup;
    if (!QFile::exists(popupPath)) return;

    auto* win  = new QWidget(nullptr, Qt::Window | Qt::WindowStaysOnTopHint);
    win->setWindowTitle("Extension Popup");
    win->setAttribute(Qt::WA_DeleteOnClose);
    win->resize(320, 400);
    auto* lay  = new QVBoxLayout(win);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* view = new QWebEngineView(win);
    view->setUrl(QUrl::fromLocalFile(popupPath));
    lay->addWidget(view);
    win->show();
}

// ── ExtensionPanel ────────────────────────────────────────────────────────────
ExtensionPanel::ExtensionPanel(Database* db, ExtensionManager* mgr,
                               QWebEngineProfile* profile, QWidget* parent)
    : QWidget(parent), m_db(db), m_mgr(mgr), m_profile(profile)
{
    setObjectName("rightPanel");

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Header
    auto* hdr = new QHBoxLayout;
    hdr->setContentsMargins(14, 12, 14, 8);
    auto* t = new QLabel("Расширения");
    t->setObjectName("panelHead");
    hdr->addWidget(t);
    lay->addLayout(hdr);

    // CWS button
    auto* cws = new QPushButton("🌐  Chrome Web Store");
    cws->setStyleSheet(
        "margin:0 12px 4px;background:rgba(10,122,255,0.12);"
        "color:#0a7aff;border:none;border-radius:9px;"
        "font-size:12px;font-weight:500;padding:9px;");
    connect(cws, &QPushButton::clicked, this, &ExtensionPanel::openCws);
    lay->addWidget(cws);

    auto mkBtn = [&](const QString& label) -> QPushButton* {
        auto* b = new QPushButton(label);
        b->setStyleSheet(
            "margin:4px 12px;background:rgba(128,128,128,0.10);"
            "border:none;border-radius:9px;"
            "font-size:12px;font-weight:500;padding:9px;");
        return b;
    };

    auto* local = mkBtn("📁  Установить из .crx файла");
    connect(local, &QPushButton::clicked, this, &ExtensionPanel::installLocal);
    lay->addWidget(local);

    auto* unpacked = mkBtn("📂  Установить распакованное");
    connect(unpacked, &QPushButton::clicked, this, &ExtensionPanel::installUnpacked);
    lay->addWidget(unpacked);

    auto* note = new QLabel("Для CWS: открой страницу расширения\nи нажми «Add to Chrome»");
    note->setObjectName("hint");
    note->setWordWrap(true);
    note->setStyleSheet("padding:4px 14px 8px 14px;");
    lay->addWidget(note);

    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background:rgba(128,128,128,0.14);max-height:1px;");
    lay->addWidget(sep);

    m_list = new QListWidget;
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_list, &QListWidget::customContextMenuRequested,
            this, &ExtensionPanel::showContextMenu);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &ExtensionPanel::onItemDoubleClicked);
    lay->addWidget(m_list);

    refresh();
}

void ExtensionPanel::setOpenTabFn(std::function<void(const QString&)> fn) {
    m_openTabFn = fn;
}

void ExtensionPanel::refresh() {
    m_list->clear();
    for (const auto& ext : m_db->getExtensions()) {
        const QString short_path = ext.path.length() > 27
            ? "…" + ext.path.right(24) : ext.path;
        auto* it = new QListWidgetItem(
            QString("%1  %2\n   %3")
                .arg(ext.enabled ? "✓" : "○")
                .arg(ext.name)
                .arg(short_path));
        it->setData(Qt::UserRole, QVariant::fromValue(QList<QVariant>{
            ext.id, ext.path, ext.enabled
        }));
        m_list->addItem(it);
    }
}

void ExtensionPanel::openCws() {
    if (m_openTabFn) m_openTabFn("https://chromewebstore.google.com/");
}

void ExtensionPanel::installLocal() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Выбери .crx файл", {},
        "Chrome Extension (*.crx)");
    if (path.isEmpty()) return;

    const auto result = m_mgr->installFromFile(path);
    if (!result.ok) {
        QMessageBox::warning(this, "Fusion", result.error);
        return;
    }
    registerExt(result.name, result.path);
}

void ExtensionPanel::installUnpacked() {
    const QString path = QFileDialog::getExistingDirectory(this, "Папка расширения");
    if (path.isEmpty()) return;

    QFile mf(path + "/manifest.json");
    if (!mf.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Fusion", "manifest.json не найден.");
        return;
    }
    auto doc = QJsonDocument::fromJson(mf.readAll());
    const QString name = doc.isObject()
        ? doc.object().value("name").toString()
        : QFileInfo(path).fileName();
    registerExt(name, path);
}

void ExtensionPanel::installFromCwsUrl(const QString& url) {
    auto* dlg = new QProgressDialog("Устанавливаю расширение…", "Отмена", 0, 0, this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setMinimumDuration(0);
    dlg->show();

    m_mgr->installFromCws(url,
        [this, dlg](const QString& path, const QString& name) {
            dlg->close();
            dlg->deleteLater();
            registerExt(name, path);
        },
        [this, dlg](const QString& err) {
            dlg->close();
            dlg->deleteLater();
            QMessageBox::warning(this, "Fusion", "Ошибка установки:\n" + err);
        });
}

void ExtensionPanel::registerExt(const QString& name, const QString& path) {
    m_db->addExtension(name, path);
    m_mgr->loadExt(path);
    refresh();
    emit extChanged();
    QMessageBox::information(this, "Fusion",
        QString("✓ «%1» установлено.\nПерезапусти браузер для полного применения.").arg(name));
}

void ExtensionPanel::showContextMenu(const QPoint& pos) {
    auto* item = m_list->itemAt(pos);
    if (!item) return;

    const auto data    = item->data(Qt::UserRole).toList();
    const int    eid   = data[0].toInt();
    const QString path = data[1].toString();
    const bool   en    = data[2].toBool();

    QMenu m(this);
    auto* popupAct  = m.addAction("🪟  Открыть popup");
    auto* toggleAct = m.addAction(en ? "✓  Выключить" : "○  Включить");
    m.addSeparator();
    auto* removeAct = m.addAction("🗑  Удалить");

    auto* act = m.exec(m_list->mapToGlobal(pos));
    if (act == popupAct)  { m_mgr->openPopup(path, this); }
    else if (act == toggleAct) { m_db->toggleExtension(eid, !en); refresh(); }
    else if (act == removeAct) { m_db->removeExtension(eid); refresh(); emit extChanged(); }
}

void ExtensionPanel::onItemDoubleClicked(QListWidgetItem* item) {
    const auto data = item->data(Qt::UserRole).toList();
    m_mgr->openPopup(data[1].toString(), this);
}
