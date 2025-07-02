#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QWidget window;

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout();

    // First grid layout
    QGridLayout* grid1 = new QGridLayout();
    grid1->addWidget(new QPushButton("Button 1"), 0, 0);
    grid1->addWidget(new QPushButton("Button 2"), 0, 1);
    grid1->addWidget(new QPushButton("Button 3"), 1, 0, 1, 2); // Spanning 2 columns

    // Second grid layout
    QGridLayout* grid2 = new QGridLayout();
    grid2->addWidget(new QPushButton("Button A"), 0, 0);
    grid2->addWidget(new QPushButton("Button B"), 0, 1);
    grid2->addWidget(new QPushButton("Button C"), 1, 0);
    grid2->addWidget(new QPushButton("Button D"), 1, 1);

    // Add grids to main layout
    mainLayout->addLayout(grid1);  // First grid
    mainLayout->addLayout(grid2);  // Second grid

    window.setLayout(mainLayout);
    window.setWindowTitle("Multiple Grids Example");
    window.show();

    return app.exec();
}
