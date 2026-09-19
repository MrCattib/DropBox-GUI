#include <QApplication>
#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineDownloadRequest>
#include <QFileDialog>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QCoreApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QProcessEnvironment>
#include <QByteArray>
#include <cstdlib>


/*
 *    =========================================
 *    Force .libs into the runtime library path
 *    =========================================
 *
 *    This is done BEFORE QApplication is created.
 */
static void forceLocalLibraries(
    const QString &libsPath
)
{
    const QByteArray libs =
    libsPath.toLocal8Bit();

    const QByteArray oldPath =
    qgetenv("LD_LIBRARY_PATH");

    QByteArray newPath = libs;

    if (!oldPath.isEmpty()) {
        newPath += ":";
        newPath += oldPath;
    }

    qputenv(
        "LD_LIBRARY_PATH",
        newPath
    );
}


/*
 *    =========================================
 *    Missing .libs warning
 *    =========================================
 */

static bool showMissingLibsWindow(
    const QString &iconPath
)
{
    QDialog dialog;

    dialog.setWindowTitle(
        "DropBox - Required Files Missing"
    );

    dialog.setWindowIcon(
        QIcon(iconPath)
    );

    dialog.setFixedSize(
        560,
        245
    );

    dialog.setSizeGripEnabled(false);

    auto *layout =
    new QVBoxLayout(&dialog);

    layout->setContentsMargins(
        25,
        22,
        25,
        20
    );

    layout->setSpacing(15);


    auto *title =
    new QLabel(
        "<b>DropBox uses .libs directory.</b>"
    );

    title->setWordWrap(true);

    layout->addWidget(title);


    auto *message =
    new QLabel(
        "This directory has REQUIRED libraries "
        "for DropBox to work properly.<br><br>"
        "We couldn't found this directory so you "
        "need to reinstall dropbox-client from a "
        ".deb package or a .tar archive."
    );

    message->setWordWrap(true);

    layout->addWidget(message);


    layout->addStretch();


    auto *buttonLayout =
    new QHBoxLayout;

    buttonLayout->addStretch();


    auto *understood =
    new QPushButton(
        "Understood"
    );

    understood->setDefault(true);

    buttonLayout->addWidget(
        understood
    );


    layout->addLayout(
        buttonLayout
    );


    QObject::connect(
        understood,
        &QPushButton::clicked,
        &dialog,
        &QDialog::accept
    );


    dialog.exec();

    return true;
}


class PersistentWebPage : public QWebEnginePage
{
public:
    explicit PersistentWebPage(
        QWebEngineProfile *profile,
        QObject *parent = nullptr
    )
    : QWebEnginePage(
        profile,
        parent
    )
    {
        connect(
            this,
            &QWebEnginePage::loadFinished,
            this,
            [this](bool ok)
            {
                if (ok)
                    injectSmoothScroll();
            }
        );
    }

protected:
    QStringList chooseFiles(
        FileSelectionMode mode,
        const QStringList &oldFiles,
        const QStringList &acceptedMimeTypes
    ) override
    {
        Q_UNUSED(oldFiles);
        Q_UNUSED(acceptedMimeTypes);

        switch (mode)
        {
            case FileSelectOpen:
            {
                const QString file =
                QFileDialog::getOpenFileName(
                    nullptr,
                    "Select File"
                );

                if (!file.isEmpty())
                    return {file};

                break;
            }

            case FileSelectOpenMultiple:
            {
                return QFileDialog::getOpenFileNames(
                    nullptr,
                    "Select Files"
                );
            }

            case FileSelectUploadFolder:
            {
                const QString folder =
                QFileDialog::getExistingDirectory(
                    nullptr,
                    "Select Folder"
                );

                if (!folder.isEmpty())
                    return {folder};

                break;
            }

            case FileSelectSave:
            {
                const QString file =
                QFileDialog::getSaveFileName(
                    nullptr,
                    "Save File"
                );

                if (!file.isEmpty())
                    return {file};

                break;
            }

            default:
                break;
        }

        return {};
    }


    QWebEnginePage *createWindow(
        WebWindowType type
    ) override
    {
        Q_UNUSED(type);

        auto *view =
        new QWebEngineView;

        view->setAttribute(
            Qt::WA_DeleteOnClose
        );

        view->resize(
            1000,
            700
        );

        view->setWindowTitle(
            "DropBox"
        );

        view->setWindowIcon(
            QIcon(
                QCoreApplication::
                applicationDirPath()
                + "/icon.png"
            )
        );


        /*
         *            Same profile means authentication,
         *            cookies and sessions are shared.
         */
        auto *page =
        new PersistentWebPage(
            profile(),
                              view
        );

        view->setPage(
            page
        );


        connect(
            page,
            &QWebEnginePage::titleChanged,
            view,
            [view](const QString &title)
            {
                if (!title.isEmpty())
                    view->setWindowTitle(
                        title
                    );
                else
                    view->setWindowTitle(
                        "DropBox"
                    );
            }
        );


        view->show();

        return page;
    }


private:
    void injectSmoothScroll()
    {
        const QString css = R"CSS(
/* =========================================
 *   Global smooth scrolling
 * ========================================= */

:root {
  --scrollbar-size: 5px;
  --scrollbar-thumb: rgba(100, 100, 100, 0.65);
  --scrollbar-thumb-hover: rgba(70, 70, 70, 0.9);
  --scrollbar-track: transparent;
  --scroll-behavior: smooth;
}

@media (prefers-color-scheme: dark) {
  :root {
    --scrollbar-thumb: rgba(190, 190, 190, 0.55);
    --scrollbar-thumb-hover: rgba(220, 220, 220, 0.85);
  }
}

html {
  scroll-behavior: var(--scroll-behavior);
  scrollbar-gutter: stable;
  overscroll-behavior-y: none;
}

body {
    min-height: 100%;
    overflow-x: hidden;
    -webkit-overflow-scrolling: touch;
}

@media (prefers-reduced-motion: reduce) {
    html {
        scroll-behavior: auto;
    }
}

/* =========================================
 *   Chromium, Chrome, Edge, Opera, Safari
 * ========================================= */

*::-webkit-scrollbar {
    width: var(--scrollbar-size);
    height: var(--scrollbar-size);
}

*::-webkit-scrollbar-track {
    background: var(--scrollbar-track);
    border: 0;
}

*::-webkit-scrollbar-thumb {
    min-height: 40px;
    background: var(--scrollbar-thumb);
    border: 0;
    border-radius: 999px;
    transition: background 0.2s ease;
}

*::-webkit-scrollbar-thumb:hover,
*::-webkit-scrollbar-thumb:active {
    background: var(--scrollbar-thumb-hover);
}

*::-webkit-scrollbar-corner {
    background: transparent;
}

*::-webkit-resizer {
    background: transparent;
}

*::-webkit-scrollbar-button {
    display: none;
    width: 0;
    height: 0;
}

/* =========================================
 *   Firefox
 * ========================================= */

* {
    scrollbar-width: thin;
    scrollbar-color: var(--scrollbar-thumb)
    var(--scrollbar-track);
}

/* =========================================
 *   Scrollable elements
 * ========================================= */

html,
body,
main,
section,
article,
aside,
div,
.modal,
.dropdown,
.drawer,
.sidebar,
.scroll-container {
    overscroll-behavior: contain;
}

.smooth-scroll {
    overflow: auto;
    scroll-behavior: smooth;
    -webkit-overflow-scrolling: touch;
    scrollbar-gutter: stable;
}

.no-horizontal-scroll {
    overflow-x: hidden;
}

.scroll-lock {
    overflow: hidden !important;
    touch-action: none;
}
)CSS";


const QString cssBase64 =
css.toUtf8().toBase64();


const QString js = R"JS(
(() => {
    const cssBase64 = "%1";

    const cssText =
        new TextDecoder().decode(
            Uint8Array.from(
                atob(cssBase64),
                c => c.charCodeAt(0)
            )
        );

    const styleId =
        "__dropbox_smooth_scroll_css";

    if (!document.getElementById(styleId)) {
        const style =
            document.createElement("style");

        style.id = styleId;
        style.textContent = cssText;

        (
            document.head ||
            document.documentElement
        ).appendChild(style);
    }


    const prefersReducedMotion =
    window.matchMedia(
        "(prefers-reduced-motion: reduce)"
    ).matches;


    /* =========================================
     *       Smooth anchor scrolling
     *    ========================================= */

    if (
        !document.documentElement.dataset
        .dropboxAnchorScroll
    ) {
        document.documentElement.dataset
        .dropboxAnchorScroll = "true";


        document.addEventListener(
            "click",
            (event) =>
            {
                const link =
                event.target.closest(
                    'a[href^="#"]'
                );

                if (!link)
                    return;


                const targetId =
                link.getAttribute("href");


                if (
                    !targetId ||
                    targetId === "#"
                )
                    return;


                    let target;

                    try {
                        target =
                        document.querySelector(
                            targetId
                        );
                    }
                    catch (e) {
                        return;
                    }


                    if (!target)
                        return;


                event.preventDefault();


                const header =
                document.querySelector(
                    "header, .header, .navbar, .nav, .fixed-header"
                );


                const headerHeight =
                header
                ? header.offsetHeight
                : 0;


                const targetPosition =
                target.getBoundingClientRect()
                .top +
                window.pageYOffset -
                headerHeight -
                16;


                window.scrollTo({
                    top: Math.max(
                        0,
                        targetPosition
                    ),

                    behavior:
                    prefersReducedMotion
                    ? "auto"
                    : "smooth"
                });


                history.pushState(
                    null,
                    "",
                    targetId
                );
            }
        );
    }


    /* =========================================
     *       Smooth mouse-wheel scrolling
     *    ========================================= */

    if (
        !document.documentElement.dataset
        .dropboxWheelScroll
    ) {
        document.documentElement.dataset
        .dropboxWheelScroll = "true";


        const getScrollParent =
        (element) =>
        {
            let current = element;


            while (
                current &&
                current !== document.body
            ) {
                const style =
                window.getComputedStyle(
                    current
                );


                const canScroll =
                (
                    style.overflowY ===
                    "auto" ||
                    style.overflowY ===
                    "scroll"
                ) &&
                current.scrollHeight >
                current.clientHeight;


                if (canScroll)
                    return current;


                current =
                current.parentElement;
            }


            return (
                document.scrollingElement ||
                document.documentElement
            );
        };


        document.addEventListener(
            "wheel",
            (event) =>
            {
                if (event.ctrlKey)
                    return;


                if (prefersReducedMotion)
                    return;


                const target =
                event.target instanceof Element
                ? event.target
                : document.documentElement;


                const scrollParent =
                getScrollParent(target);


                if (!scrollParent)
                    return;


                const maxScroll =
                scrollParent.scrollHeight -
                scrollParent.clientHeight;


                if (maxScroll <= 0)
                    return;


                const currentPosition =
                scrollParent.scrollTop;


                const newPosition =
                Math.max(
                    0,
                    Math.min(
                        maxScroll,
                        currentPosition +
                        event.deltaY
                    )
                );


                if (
                    newPosition ===
                    currentPosition
                )
                    return;


                    scrollParent.scrollTo({
                        top: newPosition,
                        left:
                        scrollParent.scrollLeft,
                        behavior: "smooth"
                    });


                    event.preventDefault();
            },
            {
                passive: false,
                capture: true
            }
        );
    }
})();
)JS";


const QString finalJs =
js.arg(cssBase64);


runJavaScript(
    finalJs
);
    }
};


class DropBoxWindow : public QMainWindow
{
public:
    explicit DropBoxWindow(
        QWebEngineProfile *profile,
        const QIcon &appIcon
    )
    : QMainWindow(),
    tray(
        new QSystemTrayIcon(this)
    )
    {
        setWindowTitle(
            "DropBox"
        );

        setWindowIcon(
            appIcon
        );

        resize(
            1200,
            800
        );


        view =
        new QWebEngineView(this);

        view->setWindowIcon(
            appIcon
        );


        auto *page =
        new PersistentWebPage(
            profile,
            view
        );


        view->setPage(
            page
        );


        setCentralWidget(
            view
        );


        view->settings()->setAttribute(
            QWebEngineSettings::
            JavascriptEnabled,
            true
        );


        view->settings()->setAttribute(
            QWebEngineSettings::
            LocalStorageEnabled,
            true
        );


        view->settings()->setAttribute(
            QWebEngineSettings::
            LocalContentCanAccessRemoteUrls,
            true
        );


        view->settings()->setAttribute(
            QWebEngineSettings::
            JavascriptCanAccessClipboard,
            true
        );


        view->settings()->setAttribute(
            QWebEngineSettings::
            JavascriptCanOpenWindows,
            true
        );


        page->load(
            QUrl(
                "https://www.dropbox.com/home"
            )
        );


        setupTray(
            appIcon
        );
    }


protected:
    void closeEvent(
        QCloseEvent *event
    ) override
    {
        if (tray->isVisible()) {
            hide();
            event->ignore();
        }
        else {
            event->accept();
        }
    }


private:
    QWebEngineView *view;
    QSystemTrayIcon *tray;


    void setupTray(
        const QIcon &appIcon
    )
    {
        auto *menu =
        new QMenu;


        auto *openAction =
        new QAction(
            "Open DropBox",
            menu
        );


        auto *quitAction =
        new QAction(
            "Quit DropBox",
            menu
        );


        connect(
            openAction,
            &QAction::triggered,
            this,
            [this]
            {
                show();
                raise();
                activateWindow();
            }
        );


        connect(
            quitAction,
            &QAction::triggered,
            qApp,
            &QApplication::quit
        );


        menu->addAction(
            openAction
        );

        menu->addSeparator();

        menu->addAction(
            quitAction
        );


        tray->setIcon(
            appIcon
        );

        tray->setContextMenu(
            menu
        );

        tray->setToolTip(
            "DropBox"
        );


        if (
            QSystemTrayIcon::
            isSystemTrayAvailable()
        ) {
            tray->show();
        }
    }
};


int main(
    int argc,
    char *argv[]
)
{
    /*
     *        =========================================
     *        IMPORTANT:
     *        Check .libs BEFORE QApplication starts.
     *        =========================================
     */

    const QString applicationDir =
    QFileInfo(
        QString::fromLocal8Bit(
            argv[0]
        )
    ).absolutePath();


    const QString libsPath =
    QDir(
        applicationDir
    ).absoluteFilePath(
        ".libs"
    );


    const QString iconPath =
    QDir(
        applicationDir
    ).absoluteFilePath(
        "icon.png"
    );


    /*
     *        =========================================
     *        .libs IS REQUIRED
     *        =========================================
     */

    if (
        !QDir(libsPath).exists()
    ) {
        /*
         *            QApplication is still required to show
         *            the warning window.
         */

        QApplication errorApp(
            argc,
            argv
        );

        errorApp.setApplicationName(
            "DropBox"
        );

        errorApp.setApplicationDisplayName(
            "DropBox"
        );

        errorApp.setWindowIcon(
            QIcon(iconPath)
        );


        showMissingLibsWindow(
            iconPath
        );


        /*
         *            Understood closes the application
         *            completely.
         */
        return 0;
    }


    /*
     *        =========================================
     *        Force .libs BEFORE QApplication
     *        =========================================
     */

    forceLocalLibraries(
        libsPath
    );


    QApplication app(
        argc,
        argv
    );


    /*
     *        =========================================
     *        Application identity
     *        =========================================
     */

    app.setApplicationName(
        "DropBox"
    );

    app.setApplicationDisplayName(
        "DropBox"
    );

    app.setDesktopFileName(
        "dropbox-client"
    );


    /*
     *        =========================================
     *        Application icon
     *        =========================================
     */

    const QIcon appIcon(
        iconPath
    );

    app.setWindowIcon(
        appIcon
    );


    /*
     *        =========================================
     *        Persistent data
     *        =========================================
     */

    const QString dataRoot =
    QDir::homePath() +
    "/.dropbox-client";


        const QString cachePath =
        dataRoot +
        "/cache";


        const QString storagePath =
        dataRoot +
        "/storage";


        const QString downloadsPath =
        QDir::homePath() +
        "/Downloads";


        QDir().mkpath(
            cachePath
        );

        QDir().mkpath(
            storagePath
        );

        QDir().mkpath(
            downloadsPath
        );


        /*
         *        =========================================
         *        Persistent WebEngine profile
         *        =========================================
         */

        auto *profile =
        new QWebEngineProfile(
            "DropBoxProfile",
            &app
        );


        profile->setPersistentStoragePath(
            storagePath
        );


        profile->setCachePath(
            cachePath
        );


        profile->setDownloadPath(
            downloadsPath
        );


        profile->setHttpCacheType(
            QWebEngineProfile::
            DiskHttpCache
        );


        profile->setPersistentCookiesPolicy(
            QWebEngineProfile::
            ForcePersistentCookies
        );


        /*
         *        =========================================
         *        Downloads
         *        =========================================
         */

        QObject::connect(
            profile,
            &QWebEngineProfile::
            downloadRequested,
            &app,
            [downloadsPath](
                QWebEngineDownloadRequest *download
            )
            {
                QString fileName =
                download->downloadFileName();


                if (fileName.isEmpty())
                    fileName = "download";


                QString fullPath =
                downloadsPath +
                "/" +
                fileName;


                QFileInfo info(
                    fullPath
                );


                int counter = 1;


                while (info.exists()) {
                    const QString base =
                    info.completeBaseName();


                    const QString suffix =
                    info.completeSuffix();


                    QString newName;


                    if (suffix.isEmpty()) {
                        newName =
                        QString(
                            "%1 (%2)"
                        )
                        .arg(base)
                        .arg(counter);
                    }
                    else {
                        newName =
                        QString(
                            "%1 (%2).%3"
                        )
                        .arg(base)
                        .arg(counter)
                        .arg(suffix);
                    }


                    fullPath =
                    downloadsPath +
                    "/" +
                    newName;


                    info.setFile(
                        fullPath
                    );


                    counter++;
                }


                download->setDownloadDirectory(
                    downloadsPath
                );


                download->setDownloadFileName(
                    QFileInfo(fullPath)
                    .fileName()
                );


                download->accept();
            }
        );


        /*
         *        =========================================
         *        Main window
         *        =========================================
         */

        DropBoxWindow window(
            profile,
            appIcon
        );


        window.show();


        return app.exec();
}
