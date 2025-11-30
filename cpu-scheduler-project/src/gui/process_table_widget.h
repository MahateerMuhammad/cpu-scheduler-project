#pragma once

#include <QWidget>
#include <QTableWidget>
#include <vector>
#include <memory>
#include "../kernel/process.h"

class ProcessTableWidget : public QTableWidget {
    Q_OBJECT

public:
    explicit ProcessTableWidget(QWidget* parent = nullptr);
    ~ProcessTableWidget();

    void updateProcessList(const std::vector<std::shared_ptr<Process>>& processes);
    int getSelectedPid() const;

private:
    void setupTable();
    QColor getStateColor(ProcessState state) const;
    QString getStateName(ProcessState state) const;
};
