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

// QtQuickRuntime provides the JavaScript API for the 
// Qt Quick Web Runtime. It has the following dependencies:
//     QtQuickRuntime.pexe
//     QtQuickRuntime.nmf
//     qtloader.js.
// (See Readme.md for build instructions)
//
// Creation and initlilization:
//  
//   var runtime = new QtQuickRuntime(config)
//   var qtQuickElement = runtime.createElement()
//   runtime.setSource(source)
//
// Supported config keys/values:
//   resizeMode    "sizeViewToRootObject"
//                 "sizeRootObjectToView" (default)
//   path          Path to the runtime implementation files
//                 (nmf and pexe), relative to the loading
//                 html file (not this script file). Default
//                 path is "".
//
// setSource() may be called again at a later point int time.
// QtQuickRuntime will then reset the view element and parse
// and display the new QML (but not reload Qt).
//
// Several events are available for tracking loading progress.
// These are provided by the element returned by createElement().
//
// Loading happens in two stages. First, the QtQuickRuntime.pexe
// binary is downloaded and processed. The wait time here can be
// in the tens of seconds. The result is cached by the browser,
// so this is usually a one-time cost where subsequent loads are
// much faster. Events are generated to track load progress:
//
//   loadstart
//   progress (Use event.loaded and event.total to determine progress)
//   loadend
//   load     iff the runtime was loaded successfully
//
// Second, the QML source code is parsed and executed. This generates
// the following events:
//
//   qmlloadstart  
//   qmlloadend     event.status: "OK" or "Error:", followed by errors
//   qmlwarnings    Will be sent if there are runtime errors
//
// A 'crash' event is sent if the Qt Quick Runtime crashes. You
// may handle this by removing the Qt Quick elment from the DOM
// and creating/loading again.
//
function QtQuickRuntime(config)
{
    // Create/update the configuration object for QtLoader. The
    // QtQuickRuntime config object is re-used: options can be
    // passed directly to QtLoader.
    config = config || {};
    config["src"] = "qtquickruntime.nmf";

    // Handle the "resizeMode" QtQuickRuntime option. Add it to the QtLoader
    // environment, which is passed to to QtQuickRuntimeInstance::Init()
    // as argn/argv arguments.
    var resizeMode = config["resizeMode"];
    if (resizeMode !== undefined) {
        if (config["environment"] === undefined)
            config["environment"] = {};
        config["environment"]["qt_qquickview_resizemode"] = resizeMode;
    }
    delete config["resizeMode"];

    // Prepend path to nmf src property if set. Take care to not introduce
    // a leading "/", this would make the src relative to the web root instead
    // of the loading document.
    var path = config["path"];
    if (path && path.substr(-path.length) !== "/")
        path += "/";
    config["src"] = (path ? path : "") + "qtquickruntime.nmf"

    var qtloader = new QtLoader(config);
    var element = undefined;
    var loadComplete = false;
    var source = undefined;
    var data = undefined;
    
    function createElement(existingElement)
    {
        element = qtloader.createElement(existingElement);

        // Set up event handling
        var capture = true;
        element.addEventListener('message', onMessage, capture);
        element.addEventListener('load', onLoad, capture);
        element.addEventListener('crash', onCrash, capture);

        return element;
    }
    
    function load()
    {
        qtloader.load();
    }
    
    function setData(newData)
    {
        source = undefined;

        if (!loadComplete) {
            data = newData;
            return;
        }

        var event = new Event('qmlloadstart');
        element.dispatchEvent(event);
        qtloader.postMessage("qt_qmldata:" + newData);
    }

    function setSource(newSource)
    {
        data = undefined;
        if (!loadComplete) {
            source = newSource;
            return;
        }

        var event = new Event('qmlloadstart');
        element.dispatchEvent(event);
        qtloader.postMessage("qt_qmlsource:" + newSource);
    }
    
    function onLoad(event) {
        loadComplete = true;

        if (source) {
            setSource(source);
            source = undefined;
        } else if (data) {
            setData(data);
            data = undefined;
        }
    }
    
    function onCrash(event) {
        loadComplete = false;
    }
    
    function onMessage(message) {
        // handle 'message' events, look for qml* messages and
        // translate to qml* events.
            
        var qmlStatusKey = "qt_qmlstatus:";
        if (message.data.indexOf(qmlStatusKey) === 0) {
            var content = message.data.slice(qmlStatusKey.length);
            var event = new CustomEvent("qmlloadend", { detail : content });
            element.dispatchEvent(event);
        }

        var qmlWarningsKey = "qt_qmlwarnings:";
        if (message.data.indexOf(qmlWarningsKey) === 0) {
            var content = message.data.slice(qmlWarningsKey.length);
            var event = new CustomEvent('qmlwarnings', { detail : content });
            element.dispatchEvent(event);
        }
    }

    // Return object containg the public API
    return {
        createElement: createElement,
        load: load,
        setData: setData,
        setSource : setSource,
    }
}