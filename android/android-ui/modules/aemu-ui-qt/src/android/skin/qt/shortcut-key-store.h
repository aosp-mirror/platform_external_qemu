// Copyright (C) 2015 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/skin/qt/logging-category.h"

#include <QKeySequence>
#include <QString>
#include <QTextStream>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QtDebug>

// Provides a generic way of mapping keyboard shortcuts to commands.
// CommandType must be DefaultConstructable.
template <class CommandType>
class ShortcutKeyStore {
    using Container = QMap<QKeySequence, CommandType>;
    using ReverseContainer = QMap<CommandType, QVector<QKeySequence>>;
    using ExternalSet = QSet<QKeySequence>;
public:
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    // Initialize the shortcut key store from a text stream.
    // The stream should contain zero or more shortcut specifications.
    // A shortcut specification maps a key sequence to a command and
    // looks like this:
    //  <KEY_SEQUENCE> <COMMAND>
    // where <KEY_SEQUENCE> is a string representation of a key sequence
    // parsable by QKeySequence and <COMMAND> is a string parsable by
    // invoking operator() on an instance of CommandParser.
    // CommandParser is used to convert QStrings into values of CommandType.
    // Objects of type CommandParser should be callable with a QString and
    // a pointer to CommandType. The result of the call shall be "true" if
    // conversion from QString was successful, false otherwise.
    // The result of the conversion shall be placed into the object pointed
    // to by the second argument. The object pointed to by the second argument
    // shall remain unmodified if the call returns false.
    // This function will continue reading shortcut specifications from
    // the stream until the stream ends or it encounters an error.
    template <class CommandParser = bool(*)(const QString&, CommandType*)>
    void populateFromTextStream(QTextStream& stream,
                                const CommandParser& command_parser,
                                bool handled_externally=false) {
        QString key_seq_str, command_str;
        while (!stream.atEnd()) {
            stream >> key_seq_str >> command_str;
            if (key_seq_str.isEmpty() || command_str.isEmpty()) {
                continue;
            }
            auto key_seq = QKeySequence::fromString(key_seq_str);
            CommandType command;
            bool command_parse_result =
                command_parser(command_str, &command);
            if (!command_parse_result || key_seq.isEmpty()) {
                qCWarning(emu) << "Warning: Unable to parse shortcut [" << command_str << "]";
            } else {
                add(key_seq, command);
                if (handled_externally) {
                    mKeySequencesHandledExternally.insert(key_seq);
                }
            }
        }
    }

    // Explicitly maps the given key sequence to the given command.
    void add(const QKeySequence& key_seq, const CommandType &command,
             bool handled_externally=false) {
        mKeySequenceToCommandMap[key_seq] = command;
        mCommandToKeySequenceMap[command].push_back(key_seq);
        if (handled_externally) {
            mKeySequencesHandledExternally.insert(key_seq);
        }
    }

    // If the given key sequence matches that of a command in the store,
    // invokes the given handler with that command as the only argument.
    // Returns true if the handler was called, false otherwise.
    template <class CommandHandler>
    bool handle(const QKeySequence& key_seq, const CommandHandler& handler) {
        auto it = mKeySequenceToCommandMap.find(key_seq);
        bool must_call_handler =
                !mKeySequencesHandledExternally.contains(key_seq) &&
                (it != mKeySequenceToCommandMap.end());
        if (must_call_handler) {
            handler(it.value());
        }
        return must_call_handler;
    }

    // Performs a reverse lookup: return a vector with all the shortcut keys
    // (or nullptr if no shortucts correspond to the given command)
    QVector<QKeySequence>* reverseLookup(CommandType cmd) {
        auto it = mCommandToKeySequenceMap.find(cmd);
        if (it != mCommandToKeySequenceMap.end()) {
            return &(it.value());
        } else {
            return nullptr;
        }
     }

    iterator begin() { return mKeySequenceToCommandMap.begin(); }
    iterator end() { return mKeySequenceToCommandMap.end(); }
    const_iterator cbegin() { return mKeySequenceToCommandMap.cbegin(); }
    const_iterator cend() { return mKeySequenceToCommandMap.cend(); }
    const_iterator begin() const{ return mKeySequenceToCommandMap.begin(); }
    const_iterator end() const { return mKeySequenceToCommandMap.end(); }

private:
    Container mKeySequenceToCommandMap;
    ReverseContainer mCommandToKeySequenceMap;
    ExternalSet mKeySequencesHandledExternally;
};

