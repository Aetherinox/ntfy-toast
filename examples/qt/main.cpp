/*
    Copyright 2024-2024 Aetherinox
    Copyright 2013-2019 Hannah von Reth <vonreth@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>
#include <QProcess>
#include <QTimer>

#include <iostream>

#include <ntfytoastactions.h>

namespace {
    constexpr int NOTIFICATION_COUNT = 10;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QLocalServer *server = new QLocalServer();

    QObject::connect(server, &QLocalServer::newConnection, server, [server]() {
        auto sock = server->nextPendingConnection();
        sock->waitForReadyRead();
        const QByteArray rawData = sock->readAll();
        sock->deleteLater();

        const QString data =
                QString::fromWCharArray(reinterpret_cast<const wchar_t *>(rawData.constData()),
                                        rawData.size() / static_cast<int>(sizeof(wchar_t)));
        qDebug() << data;
        QMap<QString, QString> map;
        for (const auto &str : data.split(QLatin1Char(';'))) {
            const auto index = str.indexOf(QLatin1Char('='));
            if (index > 0) {
                map[str.mid(0, index)] = str.mid(index + 1);
            }
        }
        const QString action = map["action"];

        const auto ntfyAction = NtfyToastActions::getAction(action.toStdWString());

        std::wcout << qPrintable(data) << std::endl;
        std::wcout << "Action: " << qPrintable(action) << " " << static_cast<int>(ntfyAction)
                   << std::endl;

        switch (ntfyAction) {
            case NtfyToastActions::Actions::Clicked:
                break;
            case NtfyToastActions::Actions::Hidden:
                break;
            case NtfyToastActions::Actions::Dismissed:
                break;
            case NtfyToastActions::Actions::Timedout:
                break;
            case NtfyToastActions::Actions::ButtonClicked:
                break;
            case NtfyToastActions::Actions::TextEntered:
                break;
            case NtfyToastActions::Actions::Error:
                break;
        }
    });

    server->listen("foo");
    std::wcout << qPrintable(server->fullServerName()) << std::endl;

    const QString appId = "NtfyToast.Qt.Example";
    QProcess proc(&app);
    proc.start("NtfyToast.exe",
               { "-install", "NtfyToastTestQt", app.applicationFilePath(), appId });
    proc.waitForFinished();

    std::wcout << proc.exitCode() << std::endl;
    std::wcout << qPrintable(proc.readAllStandardOutput()) << std::endl;
    std::wcout << qPrintable(proc.readAllStandardError()) << std::endl;

    QTimer *timer = new QTimer(&app);

    app.connect(timer, &QTimer::timeout, timer, [&] {
        static int id = 0;

        if (id >= NOTIFICATION_COUNT) {
            timer->stop();
        }

        auto proc = new QProcess(&app);
        proc->start("NtfyToast.exe",
                    { "-t", "test", "-m", "message", "-pipename", server->fullServerName(), "-id",
                      QString::number(id++), "-appId", appId, "-application",
                      app.applicationFilePath() });

        int currentId = id;
        proc->connect(proc, QOverload<int>::of(&QProcess::finished), proc, [proc, currentId, &app] {
            std::wcout << qPrintable(proc->readAllStandardOutput()) << std::endl;
            std::wcout << qPrintable(proc->readAllStandardError()) << std::endl;

            if (proc->error() != QProcess::UnknownError) {
                std::wcout << qPrintable(proc->errorString()) << std::endl;
            }

            std::wcout << qPrintable(proc->program()) << L" Notification: " << currentId
                       << L" exited with: " << proc->exitCode() << std::endl;

            if (currentId >= NOTIFICATION_COUNT) {
                app.quit();
            }

            proc->deleteLater();
        });
    });

    timer->start(1000);

    return app.exec();
}
