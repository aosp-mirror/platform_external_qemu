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
        int port = 5554;
        if (args.length > 0) {
            port = Integer.parseInt(args[0]);
        }
        InteractiveEmulatorView emuView = new InteractiveEmulatorView(port);
        JButton rotate = new JButton("Rotate The Emulator");
        rotate.addActionListener(e->{ emuView.rotate(); });

        JFrame frame = new JFrame("DefaultEmulatorDemo");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().add(emuView, BorderLayout.CENTER);
        frame.getContentPane().add(rotate, BorderLayout.SOUTH);
        frame.setPreferredSize(new Dimension(360, 640));
        frame.setTitle("Interactive Emulator Demo.");
        frame.pack();
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}
