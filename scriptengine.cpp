/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "scriptengine.h"

#include <QFile>
#include <QScriptValueIterator>

#include <KShell>

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>

#include "appinterface.h"
#include "containment.h"
#include "widget.h"

QScriptValue constructQRectFClass(QScriptEngine *engine);

ScriptEngine::ScriptEngine(Plasma::Corona *corona, QObject *parent)
    : QScriptEngine(parent),
      m_corona(corona)
{
    Q_ASSERT(m_corona);
    AppInterface *interface = new AppInterface(corona, this);
    connect(interface, SIGNAL(print(QString)), this, SIGNAL(print(QString)));
    m_scriptSelf = newQObject(interface, QScriptEngine::QtOwnership,
                              QScriptEngine::ExcludeSuperClassProperties | QScriptEngine::ExcludeSuperClassMethods);
    kDebug( )<< "*****************************";
    setupEngine();
    connect(this, SIGNAL(signalHandlerException(QScriptValue)), this, SLOT(exception(QScriptValue)));
}

ScriptEngine::~ScriptEngine()
{
}

QScriptValue ScriptEngine::activityById(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("activityById requires an id"));
    }

    const uint id = context->argument(0).toInt32();
    ScriptEngine *env = envFor(engine);
    foreach (Plasma::Containment *c, env->m_corona->containments()) {
        if (c->id() == id && !isPanel(c)) {
            return env->wrap(c, engine);
        }
    }

    return engine->undefinedValue();
}

QScriptValue ScriptEngine::activityForScreen(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("activityForScreen requires a screen id"));
    }

    const uint screen = context->argument(0).toInt32();
    const uint desktop = context->argumentCount() > 1 ? context->argument(1).toInt32() : -1;
    ScriptEngine *env = envFor(engine);
    return env->wrap(env->m_corona->containmentForScreen(screen, desktop), engine);
}

QScriptValue ScriptEngine::newActivity(QScriptContext *context, QScriptEngine *engine)
{
    return createContainment("desktop", "desktop", context, engine);
}

QScriptValue ScriptEngine::newPanel(QScriptContext *context, QScriptEngine *engine)
{
    return createContainment("panel", "panel", context, engine);
}

QScriptValue ScriptEngine::createContainment(const QString &type, const QString &defaultPlugin,
                                             QScriptContext *context, QScriptEngine *engine)
{
    QString plugin = context->argumentCount() > 0 ? context->argument(0).toString() :
                                                    defaultPlugin;

    bool exists = false;
    const KPluginInfo::List list = Plasma::Containment::listContainmentsOfType(type);
    foreach (const KPluginInfo &info, list) {
        if (info.pluginName() == plugin) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        return context->throwError(i18n("Could not find a plugin for %1 named %2.", type, plugin));
    }


    ScriptEngine *env = envFor(engine);
    Plasma::Containment *c = env->m_corona->addContainment(plugin);
    if (c) {
        if (type == "panel") {
            // some defaults
            c->setScreen(0);
            c->setLocation(Plasma::TopEdge);
        }
        c->updateConstraints(Plasma::AllConstraints | Plasma::StartupCompletedConstraint);
        c->flushPendingConstraintsEvents();
        emit env->createPendingPanelViews();
    }

    return env->wrap(c, engine);
}

QScriptValue ScriptEngine::wrap(Plasma::Applet *w, QScriptEngine *engine)
{
    Widget *wrapper = new Widget(w);
    QScriptValue v = engine->newQObject(wrapper, QScriptEngine::ScriptOwnership,
                                        QScriptEngine::ExcludeSuperClassProperties |
                                        QScriptEngine::ExcludeSuperClassMethods);
    return v;
}

QScriptValue ScriptEngine::wrap(Plasma::Containment *c, QScriptEngine *engine)
{
    Containment *wrapper = new Containment(c);
    return wrap(wrapper, engine);
}

QScriptValue ScriptEngine::wrap(Containment *c, QScriptEngine *engine)
{
    QScriptValue v = engine->newQObject(c, QScriptEngine::ScriptOwnership,
                                        QScriptEngine::ExcludeSuperClassProperties |
                                        QScriptEngine::ExcludeSuperClassMethods);
    v.setProperty("widgetById", engine->newFunction(Containment::widgetById));
    v.setProperty("addWidget", engine->newFunction(Containment::addWidget));

    return v;
}

ScriptEngine *ScriptEngine::envFor(QScriptEngine *engine)
{
    QObject *object = engine->globalObject().toQObject();
    Q_ASSERT(object);

    AppInterface *interface = qobject_cast<AppInterface *>(object);
    Q_ASSERT(interface);

    ScriptEngine *env = qobject_cast<ScriptEngine *>(interface->parent());
    Q_ASSERT(env);

    return env;
}

QScriptValue ScriptEngine::panelById(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("activityById requires an id"));
    }

    const uint id = context->argument(0).toInt32();
    ScriptEngine *env = envFor(engine);
    foreach (Plasma::Containment *c, env->m_corona->containments()) {
        if (c->id() == id && isPanel(c)) {
            return env->wrap(c, engine);
        }
    }

    return engine->undefinedValue();
}

QScriptValue ScriptEngine::fileExists(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    if (context->argumentCount() == 0) {
        return false;
    }

    const QString path = context->argument(0).toString();
    if (path.isEmpty()) {
        return false;
    }

    QFile f(KShell::tildeExpand(path));
    return f.exists();
}

void ScriptEngine::setupEngine()
{
    QScriptValue v = globalObject();
    QScriptValueIterator it(v);
    while (it.hasNext()) {
        it.next();
        // we provide our own print implementation, but we want the rest
        if (it.name() != "print") {
            m_scriptSelf.setProperty(it.name(), it.value());
        }
    }

    m_scriptSelf.setProperty("QRectF", constructQRectFClass(this));
    m_scriptSelf.setProperty("Activity", newFunction(ScriptEngine::newActivity));
    m_scriptSelf.setProperty("Panel", newFunction(ScriptEngine::newPanel));
    m_scriptSelf.setProperty("activityById", newFunction(ScriptEngine::activityById));
    m_scriptSelf.setProperty("activityForScreen", newFunction(ScriptEngine::activityForScreen));
    m_scriptSelf.setProperty("panelById", newFunction(ScriptEngine::panelById));
    m_scriptSelf.setProperty("fileExists", newFunction(ScriptEngine::fileExists));

    setGlobalObject(m_scriptSelf);
}

bool ScriptEngine::isPanel(const Plasma::Containment *c)
{
    return c->containmentType() == Plasma::Containment::PanelContainment ||
           c->containmentType() == Plasma::Containment::CustomPanelContainment;
}

void ScriptEngine::evaluateScript(const QString &script)
{
    //kDebug() << "evaluating" << m_editor->toPlainText();
    evaluate(script);
    if (hasUncaughtException()) {
        //kDebug() << "catch the exception!";
        QString error = i18n("Error: %1 at line %2\n\nBacktrace:\n%3",
                             uncaughtException().toString(),
                             QString::number(uncaughtExceptionLineNumber()),
                             uncaughtExceptionBacktrace().join("\n  "));
        emit printError(error);
    }
}

void ScriptEngine::exception(const QScriptValue &value)
{
    //kDebug() << "exception caught!" << value.toVariant();
    emit printError(value.toVariant().toString());
}

#include "scriptengine.moc"

