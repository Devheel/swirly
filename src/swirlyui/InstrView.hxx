/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2018 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef SWIRLYUI_INSTRVIEW_HXX
#define SWIRLYUI_INSTRVIEW_HXX

#include <QWidget>

class QModelIndex;
class QTableView;

namespace swirly {
namespace ui {

class Instr;
class InstrModel;

class InstrView : public QWidget {
    Q_OBJECT

  public:
    InstrView(InstrModel& model, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
    ~InstrView() noexcept override;

    void resizeColumnsToContents();

  signals:

  private slots:
    void slotClicked(const QModelIndex& index);

  private:
    InstrModel& model_;
    QTableView* table_{nullptr};
};

} // namespace ui
} // namespace swirly

#endif // SWIRLYUI_INSTRVIEW_HXX
