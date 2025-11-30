#include "process_table_widget.h"
#include <QHeaderView>
#include <QBrush>
#include <QColor>

ProcessTableWidget::ProcessTableWidget(QWidget* parent)
    : QTableWidget(parent) {
    setupTable();
}

ProcessTableWidget::~ProcessTableWidget() {}

void ProcessTableWidget::setupTable() {
    // Set column count and headers
    setColumnCount(6);
    QStringList headers;
    headers << "PID" << "Name" << "State" << "Priority" 
            << "Remaining (ms)" << "Wait Time (ms)";
    setHorizontalHeaderLabels(headers);
    
    // Configure table properties
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    
    // Resize columns to content
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    // Alternate row colors for better readability
    setAlternatingRowColors(true);
}

void ProcessTableWidget::updateProcessList(
    const std::vector<std::shared_ptr<Process>>& processes) {
    
    // Store currently selected PID to restore after update
    int selectedPid = getSelectedPid();
    
    // Disable sorting during update
    setSortingEnabled(false);
    
    // Clear and resize table
    setRowCount(0);
    setRowCount(static_cast<int>(processes.size()));
    
    int rowToSelect = -1;  // Track which row to select
    
    // Populate table
    for (size_t i = 0; i < processes.size(); ++i) {
        const auto& proc = processes[i];
        int row = static_cast<int>(i);
        
        // Check if this is the previously selected process
        if (selectedPid >= 0 && proc->getPid() == selectedPid) {
            rowToSelect = row;
        }
        
        // PID
        QTableWidgetItem* pidItem = new QTableWidgetItem(
            QString::number(proc->getPid()));
        setItem(row, 0, pidItem);
        
        // Name
        QTableWidgetItem* nameItem = new QTableWidgetItem(
            QString::fromStdString(proc->getName()));
        setItem(row, 1, nameItem);
        
        // State
        ProcessState state = proc->getState();
        QTableWidgetItem* stateItem = new QTableWidgetItem(getStateName(state));
        setItem(row, 2, stateItem);
        
        // Priority
        QTableWidgetItem* priorityItem = new QTableWidgetItem(
            QString::number(proc->getPriority()));
        setItem(row, 3, priorityItem);
        
        // Remaining Time
        QTableWidgetItem* remainingItem = new QTableWidgetItem(
            QString::number(proc->getRemainingTime()));
        setItem(row, 4, remainingItem);
        
        // Wait Time
        QTableWidgetItem* waitItem = new QTableWidgetItem(
            QString::number(proc->waitTime));
        setItem(row, 5, waitItem);
        
        // Color code the row based on state
        QColor color = getStateColor(state);
        for (int col = 0; col < 6; ++col) {
            item(row, col)->setBackground(QBrush(color));
        }
    }
    
    // Re-enable sorting
    setSortingEnabled(true);
    
    // Restore selection if the process still exists
    if (rowToSelect >= 0) {
        selectRow(rowToSelect);
    }
}

int ProcessTableWidget::getSelectedPid() const {
    QList<QTableWidgetItem*> selected = selectedItems();
    if (selected.isEmpty()) return -1;
    
    int row = selected.first()->row();
    QTableWidgetItem* pidItem = item(row, 0);
    if (pidItem) {
        return pidItem->text().toInt();
    }
    return -1;
}

QColor ProcessTableWidget::getStateColor(ProcessState state) const {
    switch (state) {
        case ProcessState::NEW:
            return QColor(173, 216, 230); // Light blue (more saturated)
        case ProcessState::READY:
            return QColor(255, 223, 0); // Golden yellow (darker, more visible)
        case ProcessState::RUNNING:
            return QColor(144, 238, 144); // Light green (more saturated)
        case ProcessState::WAITING:
            return QColor(255, 182, 193); // Light pink (softer red)
        case ProcessState::TERMINATED:
            return QColor(169, 169, 169); // Dark gray (better contrast)
        default:
            return QColor(255, 255, 255); // White
    }
}

QString ProcessTableWidget::getStateName(ProcessState state) const {
    switch (state) {
        case ProcessState::NEW:        return "NEW";
        case ProcessState::READY:      return "READY";
        case ProcessState::RUNNING:    return "RUNNING";
        case ProcessState::WAITING:    return "WAITING";
        case ProcessState::TERMINATED: return "TERMINATED";
        default:                       return "UNKNOWN";
    }
}
