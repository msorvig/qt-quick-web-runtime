/****************************************************************************
**
** Copyright (C) 2016 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/qpeppermodule.h>
#include <QtGui/qpepperinstance.h>
#include <QtQuick>

#include <ppapi/cpp/var.h>

// A QML (re) loader implementation, which manages a QQuickView
// and provides signals for propagating status and error messages.
class QmlReloader : public QObject
{
    Q_OBJECT
public:
    enum Status {
        LoadOk,
        LoadError
    };

    QmlReloader()
    : baseUrl("/")
    , view(0)
    , component(0)
    , resizeMode(QQuickView::SizeRootObjectToView)
    {
        view = new QQuickView();
        view->setResizeMode(resizeMode);
        QObject::connect(view->engine(), SIGNAL(warnings(const QList<QQmlError> &)),
                         this, SIGNAL(qmlWarnings(const QList<QQmlError> &)));
        view->show();
    }

    void setResizeMode(QQuickView::ResizeMode mode)
    {
        resizeMode = mode;
        view->setResizeMode(resizeMode);
    }

    void setSource(const QByteArray &source)
    {
        // Clear previous content
        delete component;
        view->engine()->clearComponentCache();

        // Load new content
        component = new QQmlComponent(view->engine());
        QUrl baseUrl("/");
        component->setData(source, baseUrl);
        if (component->isLoading()) {
             QObject::connect(component, SIGNAL(statusChanged(QQmlComponent::Status)),
                              this, SLOT(continueLoading()));
         } else {
             continueLoading();
         }
    }

    QList<QQmlError> errors()
    {
        if (!component)
            return QList<QQmlError>();
        return component->errors();
    }

private Q_SLOTS:
    void continueLoading()
    {
        if (component->isError()) {
            emit statusChanged(LoadError);
        } else {
            QQuickItem *item = qobject_cast<QQuickItem *>(component->create());
            view->setContent(QUrl(), component, item);
            emit statusChanged(LoadOk);
        }
    }
Q_SIGNALS:
    void statusChanged(int status);
    void qmlWarnings(const QList<QQmlError> &);

private:
    QUrl baseUrl;
    QQuickView *view;
    QQmlComponent *component;
    QQuickView::ResizeMode resizeMode;
};

// This the Qt application instance. The entry point is
// applicationInit(), which is called by QPepperInstance
// after Qt has been initialized. QPepperInstance is a
// subclass of pp::Instance, whoose full API is available
// to this class. This is used here to override HandleMessage,
// where we handle messages for the QML loader. Messages
// in other direction sent by loadStatusChanged() and 
// qmlWarnings() using pp::Instance::PostMessage()
class QtQuickRuntimeInstance : public QObject, public QPepperInstance
{
Q_OBJECT
public:
    QtQuickRuntimeInstance(PP_Instance ppInstance)
    :QPepperInstance(ppInstance)
    ,reloader(0)
    ,resizeMode(QQuickView::SizeRootObjectToView)
    {
    }

    ~QtQuickRuntimeInstance()
    {
        delete reloader;
    }

    // Called by Qt when Qt initialization is complete.
    void applicationInit()
    {
        // create QmlReloader and set up error handling
        reloader = new QmlReloader();
        reloader->setResizeMode(resizeMode);
        QObject::connect(reloader, SIGNAL(statusChanged(int)),
                         this, SLOT(loadStatusChanged(int)));
        QObject::connect(reloader, SIGNAL(qmlWarnings(const QList<QQmlError>  &)),
                         this, SLOT(qmlWarnings(const QList<QQmlError> &)));
    }
    
    bool Init(uint32_t argc, const char *argn[], const char *argv[])
    {
        // Scan for QtQuickRuntime arguments
        for (unsigned int i = 0; i < argc; ++i) {
            // Check for resize mode default override
            if (qstrcmp(argn[i], "qt_qquickview_resizemode") == 0) {
                if (qstrcmp(argv[i], "sizeViewToRootObject") == 0)
                    resizeMode = QQuickView::SizeViewToRootObject;
                else if (qstrcmp(argv[i], "sizeRootObjectToView") == 0)
                    resizeMode = QQuickView::SizeRootObjectToView;
                else
                    qWarning() << "Unknown QQuickView Resize Mode" << argv[i];
            }
        }

        // Call Qt Init function
        return QPepperInstance::Init(argc, argn, argv);
    }

    void HandleMessage(const pp::Var &varMessage)
    {
        // Get message text
        std::string stdMessage = varMessage.AsString();
        QByteArray message(stdMessage.c_str(), stdMessage.length());

        // Update QmlReloader on source change.
        QByteArray sourceMessageKey = "qmlsource:";
        if (message.startsWith(sourceMessageKey)) {
            QByteArray source = message.mid(sourceMessageKey.length());
            if (reloader) {
                reloader->setSource(source);
            }
            return;
        }

        // Forward other messages to Qt
        QPepperInstance::HandleMessage(varMessage);
    }
private Q_SLOTS:
    void loadStatusChanged(int status) {

        // Send QML status message to the javascript host. This
        // is either "OK" or "Errors" followed by the error text(s)
        QByteArray statusMessage = "qmlstatus:"; // the message key

        if (status == QmlReloader::LoadOk) {
            statusMessage.append("OK");
        } else {
            statusMessage.append("Errors: \n");
            foreach (QQmlError error, reloader->errors()) {
                statusMessage.append(error.toString());
                statusMessage.append("\n");
            }
        }

        PostMessage(pp::Var(statusMessage.constData()));
    }

    void qmlWarnings(const QList<QQmlError> & warnings) {

        // Send QML runtime warnings to the javascript host
        QByteArray warningsMessage = "qmlwarnings:";
        foreach (QQmlError warnings, warnings) {
            warningsMessage.append(warnings.toString());
            warningsMessage.append("\n");
        }

        PostMessage(pp::Var(warningsMessage.constData()));
    }
private:
    QmlReloader *reloader;
    QQuickView::ResizeMode resizeMode;
};

// pp::Module boilerplate. There is only one module
// instance which does not do much besides creating
// the pp::Instance.
class AppModule : public QPepperModule {
public:
    bool Init()
    {
        return QPepperModule::Init();
    }

    pp::Instance* CreateInstance(PP_Instance ppInstance)
    {
        return new QtQuickRuntimeInstance(ppInstance);
    }
};

// This is the main entry point, called by the browser when
// loading has been initiated by web page JavaScript.
namespace pp {
    pp::Module * CreateModule() { return new AppModule(); }
}

#include "qtquickruntime.moc"
