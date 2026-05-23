package eu.hansolo.steelseries.extras;


/**
 *
 * @author hansolo
 */
public class Led extends javax.swing.JComponent implements java.awt.event.ActionListener
{
    // <editor-fold defaultstate="collapsed" desc="Variable declaration">
    private static final eu.hansolo.steelseries.tools.Util UTIL = eu.hansolo.steelseries.tools.Util.INSTANCE;
    private final java.awt.Rectangle INNER_BOUNDS;    
    private eu.hansolo.steelseries.tools.LedColor ledColor = eu.hansolo.steelseries.tools.LedColor.RED_LED;
    private eu.hansolo.steelseries.tools.CustomLedColor customLedColor = new eu.hansolo.steelseries.tools.CustomLedColor(java.awt.Color.RED);
    private java.awt.image.BufferedImage ledImageOff = create_LED_Image(16, 0, ledColor, eu.hansolo.steelseries.tools.LedType.ROUND);
    private java.awt.image.BufferedImage ledImageOn = create_LED_Image(16, 1, ledColor, eu.hansolo.steelseries.tools.LedType.ROUND);
    private java.awt.image.BufferedImage currentLedImage;
    private final javax.swing.Timer LED_BLINKING_TIMER = new javax.swing.Timer(500, this);
    private boolean ledBlinking;
    private boolean ledOn;
    private eu.hansolo.steelseries.tools.LedType ledType = eu.hansolo.steelseries.tools.LedType.ROUND;
    private boolean initialized;
    private final transient java.awt.event.ComponentListener COMPONENT_LISTENER = new java.awt.event.ComponentAdapter() 
    {
        @Override
        public void componentResized(java.awt.event.ComponentEvent event)
        {
            final int SIZE = getWidth() <= getHeight() ? getWidth() : getHeight();                        
            java.awt.Container parent = getParent();
            if ((parent != null) && (parent.getLayout() == null))
            {
                if (SIZE < getMinimumSize().width || SIZE < getMinimumSize().height)
                {
                    setSize(getMinimumSize().width, getMinimumSize().height);
                }
                else
                {
                    setSize(SIZE, SIZE);
                }
            }
            else
            {
                if (SIZE < getMinimumSize().width || SIZE < getMinimumSize().height)
                {            
                    setPreferredSize(getMinimumSize());            
                }
                else
                {
                    setPreferredSize(new java.awt.Dimension(SIZE, SIZE));
                }
            }

            calcInnerBounds();

            init(INNER_BOUNDS.width);
            revalidate();
            repaint(INNER_BOUNDS);

        }
    };
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="Constructor">
    public Led()
    {
        super();
        addComponentListener(COMPONENT_LISTENER);
        INNER_BOUNDS = new java.awt.Rectangle(getPreferredSize()); 
        ledBlinking = false;
        ledOn = false;
        initialized = false;                        
        init(INNER_BOUNDS.width);
    }
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="Initialization">
    private void init(final int WIDTH)
    {
        if (WIDTH <= 1)
        {
            return;
        }
        
        if (ledImageOff != null)
        {
            ledImageOff.flush();
        }
        ledImageOff = create_LED_Image(WIDTH, 0, ledColor, ledType);
        
        if (ledImageOn != null)
        {
            ledImageOn.flush();
        }
        ledImageOn = create_LED_Image(WIDTH, 1, ledColor, ledType);

        if (ledOn)
        {
            setCurrentLedImage(ledImageOn);
        }
        else
        {
            setCurrentLedImage(ledImageOff);
        }
    }
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="Visualization">
    @Override
    protected void paintComponent(java.awt.Graphics g)
    {
        if (!initialized)
        {
            return;
        }
        
        final java.awt.Graphics2D G2 = (java.awt.Graphics2D) g.create();

        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);
        //G2.setRenderingHint(java.awt.RenderingHints.KEY_RENDERING, java.awt.RenderingHints.VALUE_RENDER_QUALITY);
        //G2.setRenderingHint(java.awt.RenderingHints.KEY_DITHERING, java.awt.RenderingHints.VALUE_DITHER_ENABLE);
        //G2.setRenderingHint(java.awt.RenderingHints.KEY_ALPHA_INTERPOLATION, java.awt.RenderingHints.VALUE_ALPHA_INTERPOLATION_QUALITY);
        //G2.setRenderingHint(java.awt.RenderingHints.KEY_COLOR_RENDERING, java.awt.RenderingHints.VALUE_COLOR_RENDER_QUALITY);
        G2.setRenderingHint(java.awt.RenderingHints.KEY_STROKE_CONTROL, java.awt.RenderingHints.VALUE_STROKE_PURE);
        //G2.setRenderingHint(java.awt.RenderingHints.KEY_FRACTIONALMETRICS, java.awt.RenderingHints.VALUE_FRACTIONALMETRICS_ON);
        //G2.setRenderingHint(java.awt.RenderingHints.KEY_TEXT_ANTIALIASING, java.awt.RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        G2.translate(INNER_BOUNDS.x, INNER_BOUNDS.y); 
        
        G2.drawImage(getCurrentLedImage(), 0, 0, null);
        
        G2.translate(-INNER_BOUNDS.x, -INNER_BOUNDS.y); 
        
        G2.dispose();
    }
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="Getter and setter">
    /**
     * Returns the type of LED. Possible values are ROUND, RECT_VERTICAL and RECT_HORIZONTAL
     * @return the type of LED. Possible values are ROUND, RECT_VERTICAL and RECT_HORIZONTAL
     */
    public eu.hansolo.steelseries.tools.LedType getLedType()
    {
        return this.ledType;
    }
    
    /**
     * Sets the type of LED.
     * @param LED_TYPE Possible values are ROUND, RECT_VERTICAL and RECT_HORIZONTAL
     */
    public void setLedType(final eu.hansolo.steelseries.tools.LedType LED_TYPE)
    {
        this.ledType = LED_TYPE;
        final boolean LED_WAS_ON = currentLedImage.equals(ledImageOn) ? true : false;

        ledImageOff = create_LED_Image(getWidth(), 0, ledColor, ledType);
        ledImageOn = create_LED_Image(getWidth(), 1, ledColor, ledType);

        currentLedImage = LED_WAS_ON == true ? ledImageOn : ledImageOff;

        repaint();
    }
        
    /**
     * Returns the color of led.
     * The LedColor is not a standard color but defines a
     * color scheme for the led. The default ledcolor is RED
     * @return the selected the color for the led
     */
    public eu.hansolo.steelseries.tools.LedColor getLedColor()
    {
        return this.ledColor;
    }

    /**
     * Sets the color of the threshold led.
     * The LedColor is not a standard color but defines a
     * color scheme for the led. The default ledcolor is RED
     * @param LED_COLOR
     */
    public void setLedColor(final eu.hansolo.steelseries.tools.LedColor LED_COLOR)
    {
        if (LED_COLOR == null)
        {
            this.ledColor = eu.hansolo.steelseries.tools.LedColor.RED_LED;
        }
        else
        {
            this.ledColor = LED_COLOR;
        }
        final boolean LED_WAS_ON = currentLedImage.equals(ledImageOn) ? true : false;

        ledImageOff = create_LED_Image(getWidth(), 0, LED_COLOR, ledType);
        ledImageOn = create_LED_Image(getWidth(), 1, LED_COLOR, ledType);

        currentLedImage = LED_WAS_ON == true ? ledImageOn : ledImageOff;

        repaint();
    }

    /**
     * Returns the color that will be used to calculate the custom led color
     * @return the color that will be used to calculate the custom led color
     */
    public java.awt.Color getCustomLedColor()
    {
        return this.customLedColor.COLOR;
    }
    
    /**
     * Sets the color that will be used to calculate the custom led color
     * @param COLOR 
     */
    public void setCustomLedColor(final java.awt.Color COLOR)
    {
        this.customLedColor = new eu.hansolo.steelseries.tools.CustomLedColor(COLOR);
        final boolean LED_WAS_ON = currentLedImage.equals(ledImageOn) ? true : false;

        ledImageOff = create_LED_Image(getWidth(), 0, ledColor, ledType);
        ledImageOn = create_LED_Image(getWidth(), 1, ledColor, ledType);

        currentLedImage = LED_WAS_ON == true ? ledImageOn : ledImageOff;

        repaint();
    }
    
    /**
     * Returns the object that represents the custom led color
     * @return the object that represents the custom led color
     */
    public eu.hansolo.steelseries.tools.CustomLedColor getCustomLedColorObject()
    {
        return this.customLedColor;
    }
    
    /**
     * Returns true if the led is on
     * @return true if the led is on
     */
    public boolean isLedOn()
    {
        return this.ledOn;
    }

    /**
     * Sets the state of the led
     * @param LED_ON 
     */
    public void setLedOn(final boolean LED_ON)
    {
        this.ledOn = LED_ON;
        init(getWidth());
        repaint();
    }

    /**
     * Returns the state of the threshold led.
     * The led could blink which will be triggered by a javax.swing.Timer
     * that triggers every 500 ms. The blinking will be done by switching
     * between two images.
     * @return true if the led is blinking
     */
    public boolean isLedBlinking()
    {
        return this.ledBlinking;
    }

    /**
     * Sets the state of the threshold led.
     * The led could blink which will be triggered by a javax.swing.Timer
     * that triggers every 500 ms. The blinking will be done by switching
     * between two images.
     * @param LED_BLINKING
     */
    public void setLedBlinking(final boolean LED_BLINKING)
    {
        this.ledBlinking = LED_BLINKING;
        if (LED_BLINKING)
        {
            LED_BLINKING_TIMER.start();
        }
        else
        {
            setCurrentLedImage(getLedImageOff());
            LED_BLINKING_TIMER.stop();
        }
    }
    
    /**
     * Returns the current component as buffered image.
     * To save this buffered image as png you could use for example:
     * File file = new File("image.png");
     * ImageIO.write(Image, "png", file);
     * @return the current component as buffered image
     */
    public java.awt.image.BufferedImage getAsImage()
    {
        final java.awt.image.BufferedImage IMAGE = UTIL.createImage(getWidth(), getHeight(), java.awt.Transparency.TRANSLUCENT);
        final java.awt.Graphics2D G2 = IMAGE.createGraphics();
        paintAll(G2);
        G2.dispose();
        return IMAGE;
    }
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="Image related">
    /**
     * Returns the image of the switched on threshold led
     * with the currently active ledcolor.
     * @return the image of the led with the state active
     * and the selected led color
     */
    private java.awt.image.BufferedImage getLedImageOn()
    {
        return this.ledImageOn;
    }

    /**
     * Returns the image of the switched off threshold led
     * with the currently active ledcolor.
     * @return the image of the led with the state inactive
     * and the selected led color
     */
    private java.awt.image.BufferedImage getLedImageOff()
    {
        return this.ledImageOff;
    }

    /**
     * Returns the image of the currently used led image.
     * @return the led image at the moment (depends on blinking)
     */
    private java.awt.image.BufferedImage getCurrentLedImage()
    {
        return this.currentLedImage;
    }

    /**
     * Sets the image of the currently used led image.
     * @param CURRENT_LED_IMAGE
     */
    private void setCurrentLedImage(final java.awt.image.BufferedImage CURRENT_LED_IMAGE)
    {
        this.currentLedImage = CURRENT_LED_IMAGE;
        repaint(INNER_BOUNDS);
    }

    /**
     * Returns a buffered image that represents a led with the given size, state and color
     * @param SIZE
     * @param STATE
     * @param LED_COLOR
     * @return a buffered image that represents a led with the given size, state and color
     */
    private java.awt.image.BufferedImage create_LED_Image(final int SIZE, final int STATE, final eu.hansolo.steelseries.tools.LedColor LED_COLOR, final eu.hansolo.steelseries.tools.LedType LED_TYPE)
    {
        if (SIZE <= 0)
        {
            return UTIL.createImage(1, 1, java.awt.Transparency.TRANSLUCENT);
        }
        
        final java.awt.image.BufferedImage IMAGE = UTIL.createImage(SIZE, SIZE, java.awt.Transparency.TRANSLUCENT);
        final java.awt.Graphics2D G2 = IMAGE.createGraphics();

        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);

        final int IMAGE_WIDTH = IMAGE.getWidth();
        final int IMAGE_HEIGHT = IMAGE.getHeight();
               
        // Define led data
        final java.awt.Shape LED;
        switch (LED_TYPE)
        {                            
            case RECT_VERTICAL:
                //LED = new java.awt.geom.Rectangle2D.Double(IMAGE_WIDTH * 0.39473684210526316, IMAGE_HEIGHT * 0.23684210526315788, IMAGE_WIDTH * 0.18421052631578946, IMAGE_HEIGHT * 0.5);
                LED = new java.awt.geom.Rectangle2D.Double(IMAGE_WIDTH * 0.3421052632, IMAGE_HEIGHT * 0.1052631579, IMAGE_WIDTH * 0.3157894737, IMAGE_HEIGHT * 0.7894736842);
                break;
                
            case RECT_HORIZONTAL:
                LED = new java.awt.geom.Rectangle2D.Double(IMAGE_WIDTH * 0.1052631579, IMAGE_HEIGHT * 0.3421052632, IMAGE_WIDTH * 0.7894736842, IMAGE_HEIGHT * 0.3157894737);
                break;
                
            case ROUND:
                                            
            default:
                LED = new java.awt.geom.Ellipse2D.Double(0.25 * IMAGE_WIDTH, 0.25 * IMAGE_HEIGHT, 0.5 * IMAGE_WIDTH, 0.5 * IMAGE_HEIGHT);                
                break;
        }
        
        final java.awt.geom.Ellipse2D LED_CORONA = new java.awt.geom.Ellipse2D.Double(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);

        final java.awt.geom.Point2D LED_CENTER = new java.awt.geom.Point2D.Double(LED.getBounds2D().getCenterX(), LED.getBounds2D().getCenterY());

        final float[] LED_FRACTIONS =
        {
            0.0f,
            0.2f,
            1.0f
        };

        final java.awt.Color[] LED_OFF_COLORS;
        final java.awt.Color[] LED_ON_COLORS;
        final java.awt.Color[] LED_ON_CORONA_COLORS;
        
        if (LED_COLOR != eu.hansolo.steelseries.tools.LedColor.CUSTOM)
        {
            LED_OFF_COLORS = new java.awt.Color[]
            {
                LED_COLOR.INNER_COLOR1_OFF,
                LED_COLOR.INNER_COLOR2_OFF,
                LED_COLOR.OUTER_COLOR_OFF
            };

            LED_ON_COLORS = new java.awt.Color[]
            {
                LED_COLOR.INNER_COLOR1_ON,
                LED_COLOR.INNER_COLOR2_ON,
                LED_COLOR.OUTER_COLOR_ON
            };

            LED_ON_CORONA_COLORS = new java.awt.Color[]
            {
                UTIL.setAlpha(LED_COLOR.CORONA_COLOR, 0.4f),
                UTIL.setAlpha(LED_COLOR.CORONA_COLOR, 0.4f),
                UTIL.setAlpha(LED_COLOR.CORONA_COLOR, 0.25f),
                UTIL.setAlpha(LED_COLOR.CORONA_COLOR, 0.15f),
                UTIL.setAlpha(LED_COLOR.CORONA_COLOR, 0.05f),
                UTIL.setAlpha(LED_COLOR.CORONA_COLOR, 0.0f)
            };
        }
        else
        {
            LED_OFF_COLORS = new java.awt.Color[]
            {
                customLedColor.INNER_COLOR1_OFF,
                customLedColor.INNER_COLOR2_OFF,
                customLedColor.OUTER_COLOR_OFF
            };

            LED_ON_COLORS = new java.awt.Color[]
            {
                customLedColor.INNER_COLOR1_ON,
                customLedColor.INNER_COLOR2_ON,
                customLedColor.OUTER_COLOR_ON
            };

            LED_ON_CORONA_COLORS = new java.awt.Color[]
            {
                UTIL.setAlpha(customLedColor.CORONA_COLOR, 0.4f),
                UTIL.setAlpha(customLedColor.CORONA_COLOR, 0.4f),
                UTIL.setAlpha(customLedColor.CORONA_COLOR, 0.25f),
                UTIL.setAlpha(customLedColor.CORONA_COLOR, 0.15f),
                UTIL.setAlpha(customLedColor.CORONA_COLOR, 0.05f),
                UTIL.setAlpha(customLedColor.CORONA_COLOR, 0.0f)
            };
        }
                       
        final float[] LED_INNER_SHADOW_FRACTIONS =
        {
            0.0f,
            0.8f,
            1.0f
        };

        final java.awt.Color[] LED_INNER_SHADOW_COLORS =
        {
            new java.awt.Color(0.0f, 0.0f, 0.0f, 0.0f),
            new java.awt.Color(0.0f, 0.0f, 0.0f, 0.0f),
            new java.awt.Color(0.0f, 0.0f, 0.0f, 0.4f),
        };

        final float[] LED_ON_CORONA_FRACTIONS =
        {
            0.0f,
            0.6f,
            0.7f,
            0.8f,
            0.85f,
            1.0f
        };
        
        // Define gradients for the led
        final java.awt.RadialGradientPaint LED_OFF_GRADIENT = new java.awt.RadialGradientPaint(LED_CENTER, 0.25f * IMAGE_WIDTH, LED_FRACTIONS, LED_OFF_COLORS);
        final java.awt.RadialGradientPaint LED_ON_GRADIENT = new java.awt.RadialGradientPaint(LED_CENTER, 0.25f * IMAGE_WIDTH, LED_FRACTIONS, LED_ON_COLORS);
        final java.awt.RadialGradientPaint LED_INNER_SHADOW_GRADIENT = new java.awt.RadialGradientPaint(LED_CENTER, 0.25f * IMAGE_WIDTH, LED_INNER_SHADOW_FRACTIONS, LED_INNER_SHADOW_COLORS);
        final java.awt.RadialGradientPaint LED_ON_CORONA_GRADIENT = new java.awt.RadialGradientPaint(LED_CENTER, 0.5f * IMAGE_WIDTH, LED_ON_CORONA_FRACTIONS, LED_ON_CORONA_COLORS);


        // Define light reflex data
        final java.awt.Shape LED_LIGHTREFLEX;
        final java.awt.geom.Point2D LED_LIGHTREFLEX_START;
        final java.awt.geom.Point2D LED_LIGHTREFLEX_STOP;

        switch(LED_TYPE)
        {
            case RECT_VERTICAL:
                final java.awt.geom.GeneralPath VERTICAL_HL = new java.awt.geom.GeneralPath();
                VERTICAL_HL.setWindingRule(java.awt.geom.GeneralPath.WIND_EVEN_ODD);
                VERTICAL_HL.moveTo(IMAGE_WIDTH * 0.34210526315789475, IMAGE_HEIGHT * 0.10526315789473684);
                VERTICAL_HL.lineTo(IMAGE_WIDTH * 0.6578947368421053, IMAGE_HEIGHT * 0.10526315789473684);
                VERTICAL_HL.lineTo(IMAGE_WIDTH * 0.6578947368421053, IMAGE_HEIGHT * 0.3684210526315789);
                VERTICAL_HL.curveTo(IMAGE_WIDTH * 0.6578947368421053, IMAGE_HEIGHT * 0.3684210526315789, IMAGE_WIDTH * 0.631578947368421, IMAGE_HEIGHT * 0.42105263157894735, IMAGE_WIDTH * 0.5, IMAGE_HEIGHT * 0.42105263157894735);
                VERTICAL_HL.curveTo(IMAGE_WIDTH * 0.3684210526315789, IMAGE_HEIGHT * 0.42105263157894735, IMAGE_WIDTH * 0.34210526315789475, IMAGE_HEIGHT * 0.3684210526315789, IMAGE_WIDTH * 0.34210526315789475, IMAGE_HEIGHT * 0.3684210526315789);
                VERTICAL_HL.lineTo(IMAGE_WIDTH * 0.34210526315789475, IMAGE_HEIGHT * 0.10526315789473684);
                VERTICAL_HL.closePath();
                LED_LIGHTREFLEX = VERTICAL_HL;
                LED_LIGHTREFLEX_START = new java.awt.geom.Point2D.Double(0, VERTICAL_HL.getBounds2D().getMinY() );
                LED_LIGHTREFLEX_STOP = new java.awt.geom.Point2D.Double(0, VERTICAL_HL.getBounds2D().getMaxY() );
                break;
                
            case RECT_HORIZONTAL:
                final java.awt.geom.GeneralPath HORIZONTAL_HL = new java.awt.geom.GeneralPath();
                HORIZONTAL_HL.setWindingRule(java.awt.geom.GeneralPath.WIND_EVEN_ODD);
                HORIZONTAL_HL.moveTo(IMAGE_WIDTH * 0.10526315789473684, IMAGE_HEIGHT * 0.34210526315789475);
                HORIZONTAL_HL.lineTo(IMAGE_WIDTH * 0.8947368421052632, IMAGE_HEIGHT * 0.34210526315789475);
                HORIZONTAL_HL.lineTo(IMAGE_WIDTH * 0.8947368421052632, IMAGE_HEIGHT * 0.42105263157894735);
                HORIZONTAL_HL.curveTo(IMAGE_WIDTH * 0.8947368421052632, IMAGE_HEIGHT * 0.42105263157894735, IMAGE_WIDTH * 0.7894736842105263, IMAGE_HEIGHT * 0.5, IMAGE_WIDTH * 0.5, IMAGE_HEIGHT * 0.5);
                HORIZONTAL_HL.curveTo(IMAGE_WIDTH * 0.21052631578947367, IMAGE_HEIGHT * 0.5, IMAGE_WIDTH * 0.10526315789473684, IMAGE_HEIGHT * 0.42105263157894735, IMAGE_WIDTH * 0.10526315789473684, IMAGE_HEIGHT * 0.42105263157894735);
                HORIZONTAL_HL.lineTo(IMAGE_WIDTH * 0.10526315789473684, IMAGE_HEIGHT * 0.34210526315789475);
                HORIZONTAL_HL.closePath();
                LED_LIGHTREFLEX = HORIZONTAL_HL;
                LED_LIGHTREFLEX_START = new java.awt.geom.Point2D.Double(0, LED_LIGHTREFLEX.getBounds2D().getMinY() );
                LED_LIGHTREFLEX_STOP = new java.awt.geom.Point2D.Double(0, LED_LIGHTREFLEX.getBounds2D().getMaxY() );
                break;
                
            case ROUND:
                
            default:
                LED_LIGHTREFLEX = new java.awt.geom.Ellipse2D.Double(0.4 * IMAGE_WIDTH, 0.35 * IMAGE_WIDTH, 0.2 * IMAGE_WIDTH, 0.15 * IMAGE_WIDTH);
                LED_LIGHTREFLEX_START = new java.awt.geom.Point2D.Double(0, LED_LIGHTREFLEX.getBounds2D().getMinY());
                LED_LIGHTREFLEX_STOP = new java.awt.geom.Point2D.Double(0, LED_LIGHTREFLEX.getBounds2D().getMaxY());
                break;
        }
        
        final float[] LIGHT_REFLEX_FRACTIONS =
        {
            0.0f,
            1.0f
        };

        final java.awt.Color[] LIGHTREFLEX_COLORS =
        {
            new java.awt.Color(1.0f, 1.0f, 1.0f, 0.4f),
            new java.awt.Color(1.0f, 1.0f, 1.0f, 0.0f)
        };

        // Define light reflex gradients
        final java.awt.LinearGradientPaint LED_LIGHTREFLEX_GRADIENT = new java.awt.LinearGradientPaint(LED_LIGHTREFLEX_START, LED_LIGHTREFLEX_STOP, LIGHT_REFLEX_FRACTIONS, LIGHTREFLEX_COLORS);
                        
        switch (STATE)
        {                            
            case 1:
                // LED ON
                G2.setPaint(LED_ON_CORONA_GRADIENT);
                G2.fill(LED_CORONA);
                switch (LED_TYPE)
                {
                    case ROUND:
                        G2.setPaint(LED_ON_GRADIENT);
                        G2.fill(LED);                
                        G2.setPaint(LED_INNER_SHADOW_GRADIENT);                
                        G2.fill(LED);
                        break;
                        
                    case RECT_VERTICAL:
                        G2.drawImage(eu.hansolo.steelseries.tools.Shadow.INSTANCE.createInnerShadow(LED, LED_ON_GRADIENT, 0, 0.65f, java.awt.Color.BLACK, 20, 315), (int) (IMAGE_WIDTH * 0.3421052632), (int) (IMAGE_HEIGHT * 0.1052631579), null);                    
                        break;                        
                        
                    case RECT_HORIZONTAL:                        
                        G2.drawImage(eu.hansolo.steelseries.tools.Shadow.INSTANCE.createInnerShadow(LED, LED_ON_GRADIENT, 0, 0.65f, java.awt.Color.BLACK, 20, 315), (int) (IMAGE_WIDTH * 0.1052631579), (int) (IMAGE_HEIGHT * 0.3421052632), null);                    
                        break;
                }                                
                G2.setPaint(LED_LIGHTREFLEX_GRADIENT);
                G2.fill(LED_LIGHTREFLEX);
                break;
                
            case 0:
                
            default:
                // LED OFF                
                switch (LED_TYPE)
                {
                    case ROUND:
                        G2.setPaint(LED_OFF_GRADIENT);
                        G2.fill(LED);
                        G2.setPaint(LED_INNER_SHADOW_GRADIENT);
                        G2.fill(LED);
                        break;
                        
                    case RECT_VERTICAL:
                        G2.drawImage(eu.hansolo.steelseries.tools.Shadow.INSTANCE.createInnerShadow(LED, LED_OFF_GRADIENT, 0, 0.65f, java.awt.Color.BLACK, 20, 315), (int) (IMAGE_WIDTH * 0.3421052632), (int) (IMAGE_HEIGHT * 0.1052631579), null);                    
                        break;                        
                        
                    case RECT_HORIZONTAL:                        
                        G2.drawImage(eu.hansolo.steelseries.tools.Shadow.INSTANCE.createInnerShadow(LED, LED_OFF_GRADIENT, 0, 0.65f, java.awt.Color.BLACK, 20, 315), (int) (IMAGE_WIDTH * 0.1052631579), (int) (IMAGE_HEIGHT * 0.3421052632), null);                    
                        break;
                }                                                                                                                    
                G2.setPaint(LED_LIGHTREFLEX_GRADIENT);
                G2.fill(LED_LIGHTREFLEX);                
                break;
        }

        G2.dispose();

        return IMAGE;
    }
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="Size related">
    /**
     * Calculates the rectangle that specifies the area that is available
     * for painting the gauge. This means that if the component has insets
     * that are larger than 0, these will be taken into account.
     */
    private void calcInnerBounds()
    {
        final java.awt.Insets INSETS = getInsets();        
        if (getWidth() - INSETS.left - INSETS.right < getHeight() - INSETS.top - INSETS.bottom)
        {                        
            INNER_BOUNDS.setBounds(INSETS.left, INSETS.top, getWidth() - INSETS.left - INSETS.right, getHeight() - INSETS.top - INSETS.bottom);                        
        }
        else
        {
            INNER_BOUNDS.setBounds(INSETS.left + (int) (((double) (getWidth() - INSETS.left - INSETS.right) - (double) (getHeight() - INSETS.top - INSETS.bottom)) / 2.0), INSETS.top, getHeight() - INSETS.top - INSETS.bottom, getHeight() - INSETS.top - INSETS.bottom);
        }        
    }
    
    @Override
    public java.awt.Dimension getMinimumSize()
    {
        return new java.awt.Dimension(16, 16);
    }

    @Override
    public void setPreferredSize(final java.awt.Dimension DIM)
    {        
        super.setPreferredSize(DIM);
        calcInnerBounds();
        init(DIM.width);        
        initialized = true;
        revalidate();
        repaint();
    }
    
    @Override
    public void setSize(final int WIDTH, final int HEIGHT)
    {
        super.setSize(WIDTH, WIDTH);
        calcInnerBounds();
        init(WIDTH);        
        initialized = true;
        revalidate();
        repaint();
    }
    
    @Override
    public void setSize(final java.awt.Dimension DIM)
    {
        super.setPreferredSize(DIM);
        calcInnerBounds();
        init(DIM.width);        
        initialized = true;
        revalidate();
        repaint();
    }
    
     @Override
    public void setBounds(final java.awt.Rectangle BOUNDS)
    {
        super.setBounds(new java.awt.Rectangle(BOUNDS.x, BOUNDS.y, BOUNDS.width, BOUNDS.width));
        calcInnerBounds();
        init(BOUNDS.width);        
        initialized = true;
        revalidate();
        repaint();
    }
    
    @Override
    public void setBounds(final int X, final int Y, final int WIDTH, final int HEIGHT)
    {
        super.setBounds(X, Y, WIDTH, WIDTH);
        calcInnerBounds();
        init(WIDTH);        
        initialized = true;
        revalidate();
        repaint();
    }
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="Action listener method">
    @Override
    public void actionPerformed(java.awt.event.ActionEvent event)
    {
        if (event.getSource().equals(LED_BLINKING_TIMER))
        {
            currentLedImage = ledOn == true ? getLedImageOn() : getLedImageOff();
            ledOn ^= true;

            repaint();
        }
    }
    // </editor-fold>
        
    @Override
    public String toString()
    {
        return "LED";
    }

}
