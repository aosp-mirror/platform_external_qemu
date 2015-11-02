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

#include <QKeySequence>
#include <QString>
#include <QTextStream>
#include <map>

// Provides a generic way of mapping keyboard shortcuts to commands.
// CommandType must be DefaultConstructable.
// CommandParser is used to convert QStrings into values of CommandType.
// Objects of type CommandParser should be callable with a QString and
// a pointer to CommandType. The result of the call shall be "true" if
// conversion from QString was successful, false otherwise.
// The result of the conversion shall be placed into the object pointed
// to by the second argument. The object pointed to by the second argument
// shall remain unmodified if the call returns false.
template <class CommandType, class CommandParser = bool(*)(const QString&, CommandType*)>
class ShortcutKeyStore {
    using Container = std::map<QKeySequence, CommandType>;
public:
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    explicit ShortcutKeyStore (const CommandParser& command_parser) :
        mCommandParser(command_parser) {}

    // Initialize the shortcut key store from a text stream.
    // The stream should contain zero or more shortcut specifications.
    // A shortcut specification maps a key sequence to a command and
    // looks like this:
    //  <KEY_SEQUENCE> <COMMAND>
    // where <KEY_SEQUENCE> is a string representation of a key sequence
    // parsable by QKeySequence and <COMMAND> is a string parsable by
    // a CommandParser.
    // This function will continue reading shortcut specifications from
    // the stream until the stream ends or it encounters an error.
    void populateFromTextStream(QTextStream& stream) {
        QString key_seq_str, command_str;
        while (!stream.atEnd()) {
            stream >> key_seq_str >> command_str;
            auto key_seq = QKeySequence::fromString(key_seq_str);
            CommandType command;
            bool command_parse_result =
                mCommandParser(command_str, &command);
            if (!command_parse_result || key_seq.isEmpty()) {
                break;
            } else {
                mKeySequenceToCommandMap[key_seq] = command;
            }
        }
    }

    // Explicitly maps the given key sequence to the given command.
    void add(const QKeySequence& key_seq, const CommandType &command) {
        mKeySequenceToCommandMap[key_seq] = command;
    }

    // If the given key sequence matches that of a command in the store,
    // invokes the given handler with that command as the only argument.
    // Returns true if the handler was called, false otherwise.
    template <class CommandHandler>
    bool handle(const QKeySequence& key_seq, const CommandHandler& handler) {
        auto it = mKeySequenceToCommandMap.find(key_seq);
        bool must_call_handler = (it != mKeySequenceToCommandMap.end());
        if (must_call_handler) {
            handler(it->second);
        }
        return must_call_handler;
    }

    iterator begin() { return mKeySequenceToCommandMap.begin(); }
    iterator end() { return mKeySequenceToCommandMap.end(); }
    const_iterator cbegin() { return mKeySequenceToCommandMap.cbegin(); }
    const_iterator cend() { return mKeySequenceToCommandMap.cend(); }
    const_iterator begin() const{ return mKeySequenceToCommandMap.begin(); }
    const_iterator end() const { return mKeySequenceToCommandMap.end(); }

private:
    CommandParser mCommandParser;
    Container mKeySequenceToCommandMap;
};

