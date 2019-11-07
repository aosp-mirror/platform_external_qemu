package android.emulation.studio;

import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.LayoutManager;
import javax.swing.JPanel;


/**
 * A Layout manager that keeps the aspect ration of the underlying JPanel.
 * Use this to keep the proper aspect ratio of the emulator view.
 */
public final class EmulatorLayoutManager implements LayoutManager {
    private static final int DEFAULT_PADDING = 0;
    private static final int MINIMUM_SIZE = 10;
    private final JPanel myEmulatorPanel;

    public EmulatorLayoutManager(JPanel myEmulatorPanel) {
        super();
        this.myEmulatorPanel = myEmulatorPanel;
    }

    // Both functions are empty, as we expect the component to come in
    // from the constructor.
    @Override
    public void addLayoutComponent(String name, Component comp) {

    }

    @Override
    public void removeLayoutComponent(Component comp) {

    }

    public Dimension preferredLayoutSize(Container parent) {
        int parentWidth = parent.getWidth();
        int parentHeight = parent.getHeight();
        int padding = DEFAULT_PADDING;
        Dimension preferredSize = this.myEmulatorPanel.getPreferredSize();
        if (preferredSize.getHeight() == 0 || parentHeight == 0)
            return this.minimumLayoutSize(parent);


        double aspectRatio = preferredSize.getWidth() / preferredSize.getHeight();
        double newAspectRatio = (double) parentWidth / (double) parentHeight;
        int newWidth = 0;
        int newHeight = 0;
        if (newAspectRatio > aspectRatio) {
            newHeight = parentHeight;
            newWidth = (int) (parentHeight * aspectRatio);
        } else {
            newWidth = parentWidth;
            newHeight = (int) (parentWidth / aspectRatio);
        }
        return new Dimension(Math.max(0, newWidth - padding), Math.max(0, newHeight - padding));

    }

    public Dimension minimumLayoutSize(Container parent) {
        return new Dimension(MINIMUM_SIZE, MINIMUM_SIZE);
    }

    public void layoutContainer(Container parent) {
        int parentWidth = parent.getWidth();
        int parentHeight = parent.getHeight();
        Dimension p = this.preferredLayoutSize(parent);
        this.myEmulatorPanel.setSize(p);
        this.myEmulatorPanel.setLocation((parentWidth - (int) p.getWidth()) / 2, (parentHeight - (int) p.getHeight()) / 2);
    }
}
