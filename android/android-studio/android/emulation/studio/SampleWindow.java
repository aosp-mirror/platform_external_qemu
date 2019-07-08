package android.emulation.studio;

import javax.swing.*;
import java.awt.*;

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
