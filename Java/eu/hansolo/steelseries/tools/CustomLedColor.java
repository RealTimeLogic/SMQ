package eu.hansolo.steelseries.tools;

/**
 *
 * @author Gerrit Grunwald <han.solo at muenster.de>
 */
public class CustomLedColor 
{
    public final java.awt.Color COLOR;
    public final java.awt.Color INNER_COLOR1_ON;
    public final java.awt.Color INNER_COLOR2_ON;
    public final java.awt.Color OUTER_COLOR_ON;
    public final java.awt.Color CORONA_COLOR;
    public final java.awt.Color INNER_COLOR1_OFF;
    public final java.awt.Color INNER_COLOR2_OFF;
    public final java.awt.Color OUTER_COLOR_OFF;
    
    public CustomLedColor(final java.awt.Color COLOR)
    {
        this.COLOR = COLOR;
        final float HUE = java.awt.Color.RGBtoHSB(COLOR.getRed(), COLOR.getGreen(), COLOR.getBlue(), null)[0];
        if (COLOR.getRed() == COLOR.getGreen() && COLOR.getRed() == COLOR.getBlue())
        {            
            INNER_COLOR1_ON = java.awt.Color.getHSBColor(HUE, 0.0f, 1.0f);
            INNER_COLOR2_ON = java.awt.Color.getHSBColor(HUE, 0.0f, 1.0f);
            OUTER_COLOR_ON = java.awt.Color.getHSBColor(HUE, 0.0f, 0.99f);
            CORONA_COLOR = java.awt.Color.getHSBColor(HUE, 0.0f, 1.00f);
            INNER_COLOR1_OFF = java.awt.Color.getHSBColor(HUE, 0.0f, 0.35f);
            INNER_COLOR2_OFF = java.awt.Color.getHSBColor(HUE, 0.0f, 0.35f);
            OUTER_COLOR_OFF = java.awt.Color.getHSBColor(HUE, 0.0f, 0.26f);
        }
        else
        {
            INNER_COLOR1_ON = java.awt.Color.getHSBColor(HUE, 0.75f, 1.0f);
            INNER_COLOR2_ON = java.awt.Color.getHSBColor(HUE, 0.75f, 1.0f);
            OUTER_COLOR_ON = java.awt.Color.getHSBColor(HUE, 1.0f, 0.99f);
            CORONA_COLOR = java.awt.Color.getHSBColor(HUE, 0.75f, 1.00f);
            INNER_COLOR1_OFF = java.awt.Color.getHSBColor(HUE, 1.0f, 0.35f);
            INNER_COLOR2_OFF = java.awt.Color.getHSBColor(HUE, 1.0f, 0.35f);
            OUTER_COLOR_OFF = java.awt.Color.getHSBColor(HUE, 1.0f, 0.26f);
        }
    }
}
