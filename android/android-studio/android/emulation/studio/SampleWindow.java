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

import java.util.Arrays;
import java.awt.*;
import javax.swing.*;

public class SampleWindow {

       public static Dimension parseDimension(String[] args) {
        for(int idx=0; idx < args.length; idx++) {
            if (args[idx].equalsIgnoreCase("-geometry") && idx+1 < args.length) {
                String[] dim = args[idx+1].split("[x|X]");
                return new Dimension(Integer.parseInt(dim[0]), Integer.parseInt(dim[1]));
            }
        }
        return new Dimension(360,640);
    }

    public static int parsePort(String[] args) {
         for(int idx=0; idx < args.length; idx++) {
            if (args[idx].equalsIgnoreCase("-port") && idx+1 < args.length) {
                return Integer.parseInt(args[idx+1]);
            }
        }
        return 5554;
    }

    public static void main(String[] args) {
        if (Arrays.asList(args).contains("-h")) {
            System.out.println("Usage: ");
            System.out.println("  -h                          This help message");
            System.out.println("  -noborder                   Show an undecorated window (default false)");
            System.out.println("  -geometry <int>x<int>       Initial window size (default 360x640)");
            System.out.println("  -port     <port>            Emulator telnet port (default 5554)");
            return;
        }

        int port = parsePort(args);
        Dimension size = parseDimension(args);
        boolean borderLess = Arrays.asList(args).contains("-noborder");

        InteractiveEmulatorView emuView = new InteractiveEmulatorView(port);
        EmulatorLayoutManager layoutManager = new EmulatorLayoutManager(emuView);
        JButton rotate = new JButton("Rotate The Emulator");
        JFrame frame = new JFrame("DefaultEmulatorDemo");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().add(emuView, BorderLayout.CENTER);
        frame.setLayout(layoutManager);
        frame.setPreferredSize(size);
        frame.setUndecorated(borderLess);
        frame.setTitle("Interactive Emulator Demo.");
        frame.pack();
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}
