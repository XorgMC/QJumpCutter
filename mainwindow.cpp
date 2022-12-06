#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jumpcututil.h"

#include <QFileDialog>
#include <iostream>

QStringList inputFiles;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_addInputBtn_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(tr("Videos (*.mp4 *.avi *.mkv)"));
    if (dialog.exec()) {
        foreach (QString aFile, dialog.selectedFiles()) {
            QFileInfo sf(aFile);
            ui->inputFilesList->addItem(sf.fileName());
        }
    }
    inputFiles.append(dialog.selectedFiles());
}


void MainWindow::on_remInputBtn_clicked()
{
    foreach(QListWidgetItem * anItem, ui->inputFilesList->selectedItems()) {
        ui->inputFilesList->takeItem(ui->inputFilesList->row(anItem));
    }
}


void MainWindow::on_startBtn_clicked()
{
    //foreach(QString aFile, inputFiles) {
    //}
    std::cout << "clicked" << std::endl;
    JumpCutUtil jcu;
    jcu.processVideo(inputFiles.first());
}

