/*
 * A header file to include various commonly used headers, provide common
 * forward declarations, and to provide Qt compatibility definitions.
 */
#ifndef COMMON_H
#define COMMON_H

#include <memory>

#include <QDialog>
#include <QComboBox>

class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;
class game_state;

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
constexpr void (QComboBox::*combo_activated_str) (const QString &) = &QComboBox::activated;
constexpr void (QComboBox::*combo_activated_int) (int) = &QComboBox::activated;
#else
constexpr void (QComboBox::*combo_activated_str) (const QString &) = &QComboBox::textActivated;
constexpr void (QComboBox::*combo_activated_int) (int) = &QComboBox::activated;
#endif
#endif

#include <QTextCodec>
