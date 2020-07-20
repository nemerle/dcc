#include <QtCore/QCoreApplication>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QFileInfo>

static void perform_test(QString exepath,QString filepath, QStringList args) {
    const QString tgtpathbase("./tests/outputs/"+QFileInfo(filepath).completeBaseName());

    QProcess p;
    p.setProgram(exepath);
    QStringList assembly1_args = { QString("-a 1"), QString("-o" + tgtpathbase + ".a1"), filepath };
    p.setArguments(assembly1_args);
    p.start();
    if(!p.waitForFinished(30000)) {
        qCritical() << "Timeout while calling"<<p.program()<<"with"<<p.arguments();
    }

    QStringList assembly2_args = { QString("-a 2"), QString("-o" + tgtpathbase + ".a2"), filepath };
    p.setArguments(assembly2_args);
    p.start();
    if(!p.waitForFinished(30000)) {
        qCritical() << "Timeout while calling"<<p.program()<<"with"<<p.arguments();
    }

    QStringList decompile_args;
    decompile_args.append(args);
    decompile_args.append({ QString("-o" + tgtpathbase), filepath });
    p.setArguments(decompile_args);
    p.start();
    if(!p.waitForFinished(30000)) {
        qCritical() << "Timeout while calling"<<p.program()<<"with"<<p.arguments();
    }

    qDebug().noquote().nospace() << "Tests for "<<filepath<<" done.";
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    QByteArray z;
    QString TESTS_DIR="./tests";
    QStringList test_file_filter = {QString("*.exe"),QString("*.EXE")};
    QDirIterator input_iter(TESTS_DIR+"/inputs",test_file_filter,QDir::Filter::Files,QDirIterator::Subdirectories);
    QStringList orig_args=app.arguments();

    qDebug().noquote() <<"Regression tester 0.0.2";
    orig_args.pop_front(); // remove this exe name from args
    if(orig_args.size()<2) {
        return -1;
    }
    QString dcc_exe_path = orig_args.takeFirst();
    while(input_iter.hasNext()) {
        QString next_entry = input_iter.next();
        perform_test(dcc_exe_path,next_entry,orig_args);
    }
    qDebug().noquote() <<"**************************************";
}
