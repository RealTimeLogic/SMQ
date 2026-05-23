package eu.hansolo.steelseries.tools;

/**
 * Definition of color combinations for different led colors.
 * @author hansolo
 */
public enum LedColor
{    
    RED_LED(new java.awt.Color(16751241), new java.awt.Color(16751241), new java.awt.Color(16724736), new java.awt.Color(16747888), new java.awt.Color(8264704), new java.awt.Color(8264704), new java.awt.Color(6560512)),        
    GREEN_LED(new java.awt.Color(10157961), new java.awt.Color(10157961), new java.awt.Color(5898026), new java.awt.Color(10878720), new java.awt.Color(1867264), new java.awt.Color(1867264), new java.awt.Color(1795072)),
    BLUE_LED(new java.awt.Color(9018111), new java.awt.Color(9018111), new java.awt.Color(13311), new java.awt.Color(7376383), new java.awt.Color(7294), new java.awt.Color(7294), new java.awt.Color(7012)),
    ORANGE_LED(new java.awt.Color(0xFEA23F), new java.awt.Color(0xFEA23F), new java.awt.Color(0xFD6C00), new java.awt.Color(0xFD6C00), new java.awt.Color(0x592800), new java.awt.Color(0x592800), new java.awt.Color(0x421F00)),    
    YELLOW_LED(new java.awt.Color(0xFFFF62), new java.awt.Color(0xFFFF62), new java.awt.Color(0xFFFF00), new java.awt.Color(0xFFFF00), new java.awt.Color(0x6B6D00), new java.awt.Color(0x6B6D00), new java.awt.Color(0x515300)),
    CYAN_LED(new java.awt.Color(0x00FFFF), new java.awt.Color(0x00FFFF), new java.awt.Color(0x1BC3C3), new java.awt.Color(0x00FFFF), new java.awt.Color(0x083B3B), new java.awt.Color(0x083B3B), new java.awt.Color(0x052727)),
    MAGENTA_LED(new java.awt.Color(0xD300FF), new java.awt.Color(0xD300FF), new java.awt.Color(0x8600CB), new java.awt.Color(0xC300FF), new java.awt.Color(0x38004B), new java.awt.Color(0x38004B), new java.awt.Color(0x280035)),
    RED(ColorDef.RED.LIGHTER, ColorDef.RED.LIGHTER, ColorDef.RED.DARK, ColorDef.RED.LIGHTER, ColorDef.RED.DARK, ColorDef.RED.DARK, ColorDef.RED.VERY_DARK),
    GREEN(ColorDef.GREEN.LIGHTER, ColorDef.GREEN.LIGHTER, ColorDef.GREEN.DARK, ColorDef.GREEN.LIGHTER, ColorDef.GREEN.DARK, ColorDef.GREEN.DARK, ColorDef.GREEN.VERY_DARK),
    BLUE(ColorDef.BLUE.LIGHTER, ColorDef.BLUE.LIGHTER, ColorDef.BLUE.DARK, ColorDef.BLUE.LIGHTER, ColorDef.BLUE.DARK, ColorDef.BLUE.DARK, ColorDef.BLUE.VERY_DARK),
    ORANGE(ColorDef.ORANGE.LIGHTER, ColorDef.ORANGE.LIGHTER, ColorDef.ORANGE.DARK, ColorDef.ORANGE.LIGHTER, ColorDef.ORANGE.DARK, ColorDef.ORANGE.DARK, ColorDef.ORANGE.VERY_DARK),
    YELLOW(ColorDef.YELLOW.LIGHTER, ColorDef.YELLOW.LIGHTER, ColorDef.YELLOW.DARK, ColorDef.YELLOW.LIGHTER, ColorDef.YELLOW.DARK, ColorDef.YELLOW.DARK, ColorDef.YELLOW.VERY_DARK),
    CYAN(ColorDef.CYAN.LIGHTER, ColorDef.CYAN.LIGHTER, ColorDef.CYAN.DARK, ColorDef.CYAN.LIGHTER, ColorDef.CYAN.DARK, ColorDef.CYAN.DARK, ColorDef.CYAN.VERY_DARK),
    MAGENTA(ColorDef.MAGENTA.LIGHTER, ColorDef.MAGENTA.LIGHTER, ColorDef.MAGENTA.DARK, ColorDef.MAGENTA.LIGHTER, ColorDef.MAGENTA.DARK, ColorDef.MAGENTA.DARK, ColorDef.MAGENTA.VERY_DARK),
    CUSTOM(null, null, null, null, null, null, null);    

    public final java.awt.Color INNER_COLOR1_ON;
    public final java.awt.Color INNER_COLOR2_ON;
    public final java.awt.Color OUTER_COLOR_ON;
    public final java.awt.Color CORONA_COLOR;
    public final java.awt.Color INNER_COLOR1_OFF;
    public final java.awt.Color INNER_COLOR2_OFF;
    public final java.awt.Color OUTER_COLOR_OFF;

    LedColor(final java.awt.Color INNER_COLOR1_ON, final java.awt.Color INNER_COLOR2_ON, final java.awt.Color OUTER_COLOR_ON, final java.awt.Color CORONA_COLOR, final java.awt.Color INNER_COLOR1_OFF, final java.awt.Color INNER_COLOR2_OFF, final java.awt.Color OUTER_COLOR_OFF)
    {
        this.INNER_COLOR1_ON = INNER_COLOR1_ON;
        this.INNER_COLOR2_ON = INNER_COLOR2_ON;
        this.OUTER_COLOR_ON = OUTER_COLOR_ON;
        this.CORONA_COLOR = CORONA_COLOR;
        this.INNER_COLOR1_OFF = INNER_COLOR1_OFF;
        this.INNER_COLOR2_OFF = INNER_COLOR2_OFF;
        this.OUTER_COLOR_OFF = OUTER_COLOR_OFF;
    }
}
