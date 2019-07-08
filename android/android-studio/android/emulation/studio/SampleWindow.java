// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
package android.emulation.studio;

import java.awt.*;

import javax.swing.*;

public class SampleWindow {
    public static void main(String[] args) {
        String handle = EmulatorView.DEFAULT_SHARED_MEMORY_REGION;
        if (args.length > 0) {
            handle = args[0];
        }

        JFrame frame = new JFrame("DefaultEmulatorDemo");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().add(new EmulatorView(handle), BorderLayout.CENTER);
        frame.setPreferredSize(new Dimension(360, 640));
        frame.pack();
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}
