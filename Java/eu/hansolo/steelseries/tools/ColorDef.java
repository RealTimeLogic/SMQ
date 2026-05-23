package eu.hansolo.steelseries.tools;


/**
 * Definition of colors that will be used in gradients etc.
 * This is useful to assure that you use the same color combinations
 * in all the different components. Each color is defined in three
 * brightness levels.
 * @author hansolo
 */
public enum ColorDef
{
    RED(new java.awt.Color(82, 0, 0, 255), new java.awt.Color(158, 0, 19, 255), new java.awt.Color(213, 0, 25, 255), new java.awt.Color(240, 82, 88, 255), new java.awt.Color(255, 171, 173, 255), new java.awt.Color(255, 217, 218, 255)),
    GREEN(new java.awt.Color(8, 54, 4, 255), new java.awt.Color(0, 107, 14, 255), new java.awt.Color(15, 148, 0, 255), new java.awt.Color(121, 186, 37, 255), new java.awt.Color(190, 231, 141, 255), new java.awt.Color(234, 247, 218, 255)),
    BLUE(new java.awt.Color(0, 11, 68, 255), new java.awt.Color(0, 73, 135, 255), new java.awt.Color(0, 108, 201, 255), new java.awt.Color(0, 141, 242, 255), new java.awt.Color(122, 200, 255, 255), new java.awt.Color(204, 236, 255, 255)),
    ORANGE(new java.awt.Color(118, 83, 30, 255), new java.awt.Color(215, 67, 0, 255), new java.awt.Color(240, 117, 0, 255), new java.awt.Color(255, 166, 0, 255), new java.awt.Color(255, 255, 128, 255), new java.awt.Color(255, 247, 194, 255)),
    YELLOW(new java.awt.Color(41, 41, 0, 255), new java.awt.Color(102, 102, 0, 255), new java.awt.Color(177, 165, 0, 255), new java.awt.Color(255, 242, 0, 255), new java.awt.Color(255, 250, 153, 255), new java.awt.Color(255, 252, 204, 255)),    
    CYAN(new java.awt.Color(15, 109, 109, 255), new java.awt.Color(0, 109, 144, 255), new java.awt.Color(0, 144, 191, 255), new java.awt.Color(0, 174, 239, 255), new java.awt.Color(153, 223, 249, 255), new java.awt.Color(204, 239, 252, 255)),
    MAGENTA(new java.awt.Color(98, 0, 114, 255), new java.awt.Color(128, 24, 72, 255), new java.awt.Color(191, 36, 107, 255), new java.awt.Color(255, 48, 143, 255), new java.awt.Color(255, 172, 210, 255), new java.awt.Color(255, 214, 23, 255)),
    WHITE(new java.awt.Color(210, 210, 210, 255), new java.awt.Color(220, 220, 220, 255), new java.awt.Color(235, 235, 235, 255), java.awt.Color.WHITE, java.awt.Color.WHITE, java.awt.Color.WHITE),
    GRAY(new java.awt.Color(25, 25, 25, 255), new java.awt.Color(51, 51, 51, 255), new java.awt.Color(76, 76, 76, 255), new java.awt.Color(128, 128, 128, 255), new java.awt.Color(204, 204, 204, 255), new java.awt.Color(243, 243, 243, 255)),
    BLACK(new java.awt.Color(0, 0, 0, 255), new java.awt.Color(5, 5, 5, 255), new java.awt.Color(10, 10, 10, 255), new java.awt.Color(15, 15, 15, 255), new java.awt.Color(20, 20, 20, 255), new java.awt.Color(25, 25, 25, 255)),
    RAITH(new java.awt.Color(0, 32, 65, 255), new java.awt.Color(0, 65, 125, 255), new java.awt.Color(0, 106, 172, 255), new java.awt.Color(130, 180, 214, 255), new java.awt.Color(148, 203, 242, 255), new java.awt.Color(191, 229, 255, 255)),
    GREEN_LCD(new java.awt.Color(0, 55, 45, 255), new java.awt.Color(15, 109, 93, 255), new java.awt.Color(0, 185, 165, 255), new java.awt.Color(48, 255, 204,255), new java.awt.Color(153, 255, 227, 255), new java.awt.Color(204, 255, 241, 255)),
    JUG_GREEN(new java.awt.Color(0, 56, 0, 255), new java.awt.Color(0x204524), new java.awt.Color(0x32A100), new java.awt.Color(0x81CE00), new java.awt.Color(190, 231, 141, 255), new java.awt.Color(234, 247, 218, 255)),
    CUSTOM(null, null, null, null, null, null);    

    public final java.awt.Color VERY_DARK;
    public final java.awt.Color DARK;
    public final java.awt.Color MEDIUM;
    public final java.awt.Color LIGHT;
    public final java.awt.Color LIGHTER;
    public final java.awt.Color VERY_LIGHT;
  
    ColorDef(final java.awt.Color VERY_DARK_COLOR, final java.awt.Color DARK_COLOR, final java.awt.Color MEDIUM_COLOR, final java.awt.Color LIGHT_COLOR, final java.awt.Color LIGHTER_COLOR, final java.awt.Color VERY_LIGHT_COLOR)
    {
        this.VERY_DARK = VERY_DARK_COLOR;
        this.DARK = DARK_COLOR;
        this.MEDIUM = MEDIUM_COLOR;
        this.LIGHT = LIGHT_COLOR;
        this.LIGHTER = LIGHTER_COLOR;
        this.VERY_LIGHT = VERY_LIGHT_COLOR;
    }
}
