#pragma once
#include <QString>
class QApplication;
class QVBoxLayout;
class QWidget;
class QIcon;

/// Applies the application's consistent light palette and styles.
void applyTheme(QApplication &application);
/// Adds a page title and a short user-facing explanation.
void addPageHeading(QVBoxLayout *layout, QString title, QString subtitle);
/// Shows an explicit Bulgarian confirmation.
bool confirmAction(QWidget *parent, QString title, QString message);
/// Draws a simple navigation icon without external assets.
QIcon navigationIcon(int type);
