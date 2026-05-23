package eu.hansolo.steelseries.tools;


/**
 * Definition of methods to create inner shadows and
 * drop shadows for shapes. The syntax of the methods
 * uses the same parameters as Adobe Fireworks.
 * @author hansolo
 */
public enum Shadow
{
    INSTANCE;

    private final eu.hansolo.steelseries.tools.Util UTIL = eu.hansolo.steelseries.tools.Util.INSTANCE;
    
    /**
     * <p>Return a new compatible image that contains the given shape
     * with the paint, color, stroke etc. that is specified.</p>
     * @param SHAPE the shape to create the image fromf
     * @param PAINT the paint of the shape
     * @param COLOR the color of the shape
     * @param FILLED indicator if the shape is filled or not
     * @param STROKE the stroke of the shape
     * @param STROKE_COLOR color of the stroke
     * @return a new compatible image that contains the given shape 
     */
    public java.awt.image.BufferedImage createImageFromShape(final java.awt.Shape SHAPE, final java.awt.Paint PAINT, final java.awt.Color COLOR, final boolean FILLED, final java.awt.Stroke STROKE, final java.awt.Color STROKE_COLOR)
    {        
        final java.awt.image.BufferedImage IMAGE = UTIL.createImage(SHAPE.getBounds().width, SHAPE.getBounds().height, java.awt.Transparency.TRANSLUCENT);
        final java.awt.Graphics2D G2 = IMAGE.createGraphics();

        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);

        G2.translate(-SHAPE.getBounds2D().getX(), - SHAPE.getBounds2D().getY());

        if (PAINT != null)
        {
            G2.setPaint(PAINT);
            if (FILLED)
            {
                G2.fill(SHAPE);
            }
            else
            {
                G2.draw(SHAPE);
            }
        }

        if (COLOR != null)
        {
            G2.setColor(COLOR);
            if (FILLED)
            {
                G2.fill(SHAPE);
            }
            else
            {
                G2.draw(SHAPE);
            }
        }

        if (STROKE != null)
        {
            if (STROKE_COLOR != null)
            {
                G2.setColor(STROKE_COLOR);
            }

            G2.setStroke(STROKE);

            if (!FILLED)
            {
                G2.draw(SHAPE);
            }
        }

        G2.dispose();

        return IMAGE;

    }

    public java.awt.image.BufferedImage createDropShadow(final java.awt.image.BufferedImage SRC_IMAGE, final int DISTANCE, final float ALPHA, final int SOFTNESS, final int ANGLE, final java.awt.Color SHADOW_COLOR)
    {
        final float TRANSLATE_X = (float) (DISTANCE * Math.cos(Math.toRadians(360 - ANGLE)));
        final float TRANSLATE_Y = (float) (DISTANCE * Math.sin(Math.toRadians(360 - ANGLE)));

        final java.awt.image.BufferedImage SHADOW_IMAGE = renderDropShadow(SRC_IMAGE, SOFTNESS, ALPHA, SHADOW_COLOR);
        final java.awt.image.BufferedImage RESULT = new java.awt.image.BufferedImage(SHADOW_IMAGE.getWidth(), SHADOW_IMAGE.getHeight(), java.awt.image.BufferedImage.TYPE_INT_ARGB);

        final java.awt.Graphics2D G2 = RESULT.createGraphics();
        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);
        G2.translate(TRANSLATE_X, TRANSLATE_Y);
        G2.drawImage(SHADOW_IMAGE, 0, 0, null);
        G2.translate(-TRANSLATE_X, -TRANSLATE_Y);
        G2.translate(SOFTNESS , SOFTNESS);
        G2.drawImage(SRC_IMAGE, 0, 0, null);

        G2.dispose();

        return RESULT;
    }

    public java.awt.image.BufferedImage createDropShadow(final java.awt.Shape SHAPE, final java.awt.Paint PAINT, final java.awt.Color COLOR, final boolean FILLED, final java.awt.Stroke STROKE, final java.awt.Color STROKE_COLOR, final int DISTANCE, final float ALPHA, final int SOFTNESS, final int ANGLE, final java.awt.Color SHADOW_COLOR)
    {
        final float TRANSLATE_X = (float) (DISTANCE * Math.cos(Math.toRadians(360 - ANGLE)));
        final float TRANSLATE_Y = (float) (DISTANCE * Math.sin(Math.toRadians(360 - ANGLE)));

        final java.awt.image.BufferedImage SHAPE_IMAGE = createImageFromShape(SHAPE, PAINT, COLOR, FILLED, STROKE, STROKE_COLOR);
        final java.awt.image.BufferedImage SHADOW_IMAGE = renderDropShadow(SHAPE_IMAGE, SOFTNESS, ALPHA, SHADOW_COLOR);

        final java.awt.image.BufferedImage RESULT = new java.awt.image.BufferedImage(SHADOW_IMAGE.getWidth(), SHADOW_IMAGE.getHeight(), java.awt.image.BufferedImage.TYPE_INT_ARGB);

        final java.awt.Graphics2D G2 = RESULT.createGraphics();
        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);
        G2.translate(TRANSLATE_X, TRANSLATE_Y);
        G2.drawImage(SHADOW_IMAGE, 0, 0, null);
        G2.translate(-TRANSLATE_X, -TRANSLATE_Y);
        G2.translate(SOFTNESS , SOFTNESS);
        G2.drawImage(SHAPE_IMAGE, 0, 0, null);

        G2.dispose();

        return RESULT;
    }

    /**
     * <p>Method to create a inner shadow on a given shape</p>
     * @param SOFT_CLIP_IMAGE softclipimage create by method createSoftClipImage()
     * @param SHAPE shape that should get the inner shadow
     * @param DISTANCE distance of the shadow
     * @param ALPHA alpha value of the shadow
     * @param SHADOW_COLOR color of the shadow
     * @param SOFTNESS softness/fuzzyness of the shadow
     * @param ANGLE angle under which the shadow should appear
     * @return IMAGE buffered image that contains the shape including the inner shadow
     */
    public java.awt.image.BufferedImage createInnerShadow(final java.awt.image.BufferedImage SOFT_CLIP_IMAGE, final java.awt.Shape SHAPE, final int DISTANCE, final float ALPHA, final java.awt.Color SHADOW_COLOR, final int SOFTNESS, final int ANGLE)
    {
        final float COLOR_CONSTANT = 1f / 255f;
        final float RED = COLOR_CONSTANT * SHADOW_COLOR.getRed();
        final float GREEN = COLOR_CONSTANT * SHADOW_COLOR.getGreen();
        final float BLUE = COLOR_CONSTANT * SHADOW_COLOR.getBlue();
        final float MAX_STROKE_WIDTH = SOFTNESS * 2;
        final float ALPHA_STEP = 1f / (2 * SOFTNESS + 2) * ALPHA;
        final float TRANSLATE_X = (float) (DISTANCE * Math.cos(Math.toRadians(ANGLE)));
        final float TRANSLATE_Y = (float) (DISTANCE * Math.sin(Math.toRadians(ANGLE)));

        final java.awt.Graphics2D G2 = SOFT_CLIP_IMAGE.createGraphics();

        // Enable Antialiasing
        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);

        // Translate the coordinate system to 0,0
        G2.translate(-SHAPE.getBounds2D().getX(), -SHAPE.getBounds2D().getY());

        // Set the color
        G2.setColor(new java.awt.Color(RED, GREEN, BLUE, ALPHA_STEP));

        // Set the alpha transparency of the whole image
        G2.setComposite(java.awt.AlphaComposite.SrcAtop);

        // Translate the coordinate system related to the given distance and angle
        G2.translate(TRANSLATE_X, -TRANSLATE_Y);

        // Draw the inner shadow
        for (float strokeWidth = SOFTNESS; strokeWidth >= 1; strokeWidth -= 1)
        {
            G2.setStroke(new java.awt.BasicStroke((float)(MAX_STROKE_WIDTH * Math.pow(0.85, strokeWidth))));
            G2.draw(SHAPE);
        }

        G2.dispose();

        return SOFT_CLIP_IMAGE;
    }

    /**
     * <p>Method to create a inner shadow on a given shape</p>
     * @param SHAPE shape that should get the inner shadow
     * @param SHAPE_PAINT paint of the given shape
     * @param DISTANCE distance of the shadow
     * @param ALPHA alpha value of the shadow
     * @param SHADOW_COLOR color of the shadow
     * @param SOFTNESS softness/fuzzyness of the shadow
     * @param ANGLE angle under which the shadow should appear
     * @return IMAGE buffered image that contains the shape including the inner shadow
     */
    public java.awt.image.BufferedImage createInnerShadow(final java.awt.Shape SHAPE, final java.awt.Paint SHAPE_PAINT, final int DISTANCE, final float ALPHA, final java.awt.Color SHADOW_COLOR, final int SOFTNESS, final int ANGLE)
    {
        final float COLOR_CONSTANT = 1f / 255f;
        final float RED = COLOR_CONSTANT * SHADOW_COLOR.getRed();
        final float GREEN = COLOR_CONSTANT * SHADOW_COLOR.getGreen();
        final float BLUE = COLOR_CONSTANT * SHADOW_COLOR.getBlue();
        final float MAX_STROKE_WIDTH = SOFTNESS * 2;
        final float ALPHA_STEP = 1f / (2 * SOFTNESS + 2) * ALPHA;
        final float TRANSLATE_X = (float) (DISTANCE * Math.cos(Math.toRadians(ANGLE)));
        final float TRANSLATE_Y = (float) (DISTANCE * Math.sin(Math.toRadians(ANGLE)));

        final java.awt.image.BufferedImage IMAGE = createSoftClipImage(SHAPE, SHAPE_PAINT, 0, 0, 0);

        final java.awt.Graphics2D G2 = IMAGE.createGraphics();

        // Enable Antialiasing
        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);

        // Translate the coordinate system to 0,0
        G2.translate(-SHAPE.getBounds2D().getX(), -SHAPE.getBounds2D().getY());

        // Set the color
        G2.setColor(new java.awt.Color(RED, GREEN, BLUE, ALPHA_STEP));

        // Set the alpha transparency of the whole image
        G2.setComposite(java.awt.AlphaComposite.SrcAtop);

        // Translate the coordinate system related to the given distance and angle
        G2.translate(TRANSLATE_X, -TRANSLATE_Y);

        // Draw the inner shadow
        for (float strokeWidth = SOFTNESS; strokeWidth >= 1; strokeWidth -= 1)
        {
            G2.setStroke(new java.awt.BasicStroke((float)(MAX_STROKE_WIDTH * Math.pow(0.85, strokeWidth))));
            G2.draw(SHAPE);
        }

        G2.dispose();

        return IMAGE;
    }

    /**
     * Method that creates a intermediate image to enable soft clipping functionality
     * This code was taken from Chris Campbells blog http://weblogs.java.net/blog/campbell/archive/2006/07/java_2d_tricker.html
     * @param SHAPE
     * @param SHAPE_PAINT
     * @return IMAGE buffered image that will be used for soft clipping
     */
    public java.awt.image.BufferedImage createSoftClipImage(final java.awt.Shape SHAPE, final java.awt.Paint SHAPE_PAINT)
    {        
        final java.awt.image.BufferedImage IMAGE = UTIL.createImage(SHAPE.getBounds().width, SHAPE.getBounds().height, java.awt.Transparency.TRANSLUCENT);
        final java.awt.Graphics2D G2 = IMAGE.createGraphics();

        // Clear the image so all pixels have zero alpha
        G2.setComposite(java.awt.AlphaComposite.Clear);
        G2.fillRect(0, 0, IMAGE.getWidth(), IMAGE.getHeight());

        // Render our clip shape into the image.  Note that we enable
        // antialiasing to achieve the soft clipping effect.  Try
        // commenting out the line that enables antialiasing, and
        // you will see that you end up with the usual hard clipping.
        G2.setComposite(java.awt.AlphaComposite.Src);
        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);

        // Set Color or Gradient here
        if (SHAPE_PAINT != null)
        {
            G2.setPaint(SHAPE_PAINT);
        }

        // Translate the coordinate system to 0,0
        G2.translate(-SHAPE.getBounds2D().getX(), -SHAPE.getBounds2D().getY());
        G2.fill(SHAPE);

        return IMAGE;
    }

    /**
     * <p>Method that creates a intermediate image to enable soft clipping functionality
     * This code was taken from Chris Campbells blog http://weblogs.java.net/blog/campbell/archive/2006/07/java_2d_tricker.html</p>
     * @param SHAPE
     * @param SHAPE_PAINT
     * @param SOFTNESS
     * @param TRANSLATE_X
     * @param TRANSLATE_Y
     * @return IMAGE buffered image that will be used for soft clipping
     */
    public java.awt.image.BufferedImage createSoftClipImage(final java.awt.Shape SHAPE, final java.awt.Paint SHAPE_PAINT, final int SOFTNESS, final int TRANSLATE_X, final int TRANSLATE_Y)
    {        
        final java.awt.image.BufferedImage IMAGE = UTIL.createImage(SHAPE.getBounds().width + 2 * SOFTNESS + TRANSLATE_X, SHAPE.getBounds().height + 2 * SOFTNESS + TRANSLATE_Y, java.awt.Transparency.TRANSLUCENT);
        final java.awt.Graphics2D G2 = IMAGE.createGraphics();

        G2.translate(SOFTNESS / 2.0, SOFTNESS / 2.0);

        // Clear the image so all pixels have zero alpha
        G2.setComposite(java.awt.AlphaComposite.Clear);
        G2.fillRect(0, 0, IMAGE.getWidth(), IMAGE.getHeight());

        // Render our clip shape into the image.  Note that we enable
        // antialiasing to achieve the soft clipping effect.  Try
        // commenting out the line that enables antialiasing, and
        // you will see that you end up with the usual hard clipping.
        G2.setComposite(java.awt.AlphaComposite.Src);
        G2.setRenderingHint(java.awt.RenderingHints.KEY_ANTIALIASING, java.awt.RenderingHints.VALUE_ANTIALIAS_ON);

        // Set Color or Gradient here
        if (SHAPE_PAINT != null)
        {
            G2.setPaint(SHAPE_PAINT);
        }

        // Translate the coordinate system to 0,0
        G2.translate(-SHAPE.getBounds2D().getX(), -SHAPE.getBounds2D().getY());
        G2.fill(SHAPE);

        return IMAGE;
    }

    /**
     * <p>Generates the shadow for a given picture and the current properties
     * of the renderer.</p>
     * <p>The generated image dimensions are computed as following</p>
     * <pre>
     * width  = imageWidth  + 2 * shadowSize
     * height = imageHeight + 2 * shadowSize
     * </pre>
     * @param IMAGE image the picture from which the shadow must be cast
     * @param SOFTNESS
     * @param ALPHA
     * @param SHADOW_COLOR
     * @return the picture containing the shadow of <code>image</code>
     */
    public java.awt.image.BufferedImage renderDropShadow(final java.awt.image.BufferedImage IMAGE, final int SOFTNESS, final float ALPHA, final java.awt.Color SHADOW_COLOR)
    {
        // Written by Sesbastien Petrucci
        final int SHADOW_SIZE = SOFTNESS * 2;

        final int SRC_WIDTH = IMAGE.getWidth();
        final int SRC_HEIGHT = IMAGE.getHeight();

        final int DST_WIDTH = SRC_WIDTH + SHADOW_SIZE;
        final int DST_HEIGHT = SRC_HEIGHT + SHADOW_SIZE;

        final int LEFT = SOFTNESS;
        final int RIGHT = SHADOW_SIZE - LEFT;

        final int Y_STOP = DST_HEIGHT - RIGHT;

        final int SHADOW_RGB = SHADOW_COLOR.getRGB() & 0x00FFFFFF;
        int[] aHistory = new int[SHADOW_SIZE];
        int historyIdx;

        int aSum;

        final java.awt.image.BufferedImage DST = new java.awt.image.BufferedImage(DST_WIDTH, DST_HEIGHT, java.awt.image.BufferedImage.TYPE_INT_ARGB);

        int[] dstBuffer = new int[DST_WIDTH * DST_HEIGHT];
        int[] srcBuffer = new int[SRC_WIDTH * SRC_HEIGHT];

        getPixels(IMAGE, 0, 0, SRC_WIDTH, SRC_HEIGHT, srcBuffer);

        final int LAST_PIXEL_OFFSET = RIGHT * DST_WIDTH;
        final float H_SUM_DIVIDER = 1.0f / SHADOW_SIZE;
        final float V_SUM_DIVIDER = ALPHA / SHADOW_SIZE;

        int max;

        int[] hSumLookup = new int[256 * SHADOW_SIZE];
        max = hSumLookup.length;

        for (int i = 0; i < max; i++)
        {
            hSumLookup[i] = (int) (i * H_SUM_DIVIDER);
        }

        int[] vSumLookup = new int[256 * SHADOW_SIZE];
        max = vSumLookup.length;
        for (int i = 0; i < max; i++)
        {
            vSumLookup[i] = (int) (i * V_SUM_DIVIDER);
        }

        int srcOffset;

        // horizontal pass  extract the alpha mask from the source picture and
        // blur it into the destination picture
        for (int srcY = 0, dstOffset = LEFT * DST_WIDTH; srcY < SRC_HEIGHT; srcY++)
        {
            // first pixels are empty
            for (historyIdx = 0; historyIdx < SHADOW_SIZE;)
            {
                aHistory[historyIdx++] = 0;
            }

            aSum = 0;
            historyIdx = 0;
            srcOffset = srcY * SRC_WIDTH;

            // compute the blur average with pixels from the source image
            for (int srcX = 0; srcX < SRC_WIDTH; srcX++)
            {
                int a = hSumLookup[aSum];
                dstBuffer[dstOffset++] = a << 24; // store the alpha value only
                // the shadow color will be added in the next pass

                aSum -= aHistory[historyIdx]; // substract the oldest pixel from the sum

                // extract the new pixel ...
                a = srcBuffer[srcOffset + srcX] >>> 24;
                aHistory[historyIdx] = a; // ... and store its value into history
                aSum += a; // ... and add its value to the sum

                if (++historyIdx >= SHADOW_SIZE)
                {
                    historyIdx -= SHADOW_SIZE;
                }
            }

            // blur the end of the row - no new pixels to grab
            for (int i = 0; i < SHADOW_SIZE; i++)
            {
                final int A = hSumLookup[aSum];
                dstBuffer[dstOffset++] = A << 24;

                // substract the oldest pixel from the sum ... and nothing new to add !
                aSum -= aHistory[historyIdx];

                if (++historyIdx >= SHADOW_SIZE)
                {
                    historyIdx -= SHADOW_SIZE;
                }
            }
        }

        // vertical pass
        for (int x = 0, bufferOffset = 0; x < DST_WIDTH; x++, bufferOffset = x)
        {
            aSum = 0;

            // first pixels are empty
            for (historyIdx = 0; historyIdx < LEFT;)
            {
                aHistory[historyIdx++] = 0;
            }

            // and then they come from the dstBuffer
            for (int y = 0; y < RIGHT; y++, bufferOffset += DST_WIDTH)
            {
                final int A = dstBuffer[bufferOffset] >>> 24; // extract alpha
                aHistory[historyIdx++] = A; // store into history
                aSum += A; // and add to sum
            }

            bufferOffset = x;
            historyIdx = 0;

            // compute the blur avera`ge with pixels from the previous pass
            for (int y = 0; y < Y_STOP; y++, bufferOffset += DST_WIDTH)
            {

                int a = vSumLookup[aSum];
                dstBuffer[bufferOffset] = a << 24 | SHADOW_RGB; // store alpha value + shadow color

                aSum -= aHistory[historyIdx]; // substract the oldest pixel from the sum

                a = dstBuffer[bufferOffset + LAST_PIXEL_OFFSET] >>> 24; // extract the new pixel ...
                aHistory[historyIdx] = a; // ... and store its value into history
                aSum += a; // ... and add its value to the sum

                if (++historyIdx >= SHADOW_SIZE)
                {
                    historyIdx -= SHADOW_SIZE;
                }
            }

            // blur the end of the column - no pixels to grab anymore
            for (int y = Y_STOP; y < DST_HEIGHT; y++, bufferOffset += DST_WIDTH)
            {

                final int A = vSumLookup[aSum];
                dstBuffer[bufferOffset] = A << 24 | SHADOW_RGB;

                aSum -= aHistory[historyIdx]; // substract the oldest pixel from the sum

                if (++historyIdx >= SHADOW_SIZE)
                {
                    historyIdx -= SHADOW_SIZE;
                }
            }
        }

        setPixels(DST, 0, 0, DST_WIDTH, DST_HEIGHT, dstBuffer);

        return DST;
    }

    /**
     * <p>Returns an array of pixels, stored as integers, from a
     * <code>BufferedImage</code>. The pixels are grabbed from a rectangular
     * area defined by a location and two dimensions. Calling this method on
     * an image of type different from <code>BufferedImage.TYPE_INT_ARGB</code>
     * and <code>BufferedImage.TYPE_INT_RGB</code> will unmanage the image.</p>
     *
     * @param IMAGE the source image
     * @param X the x location at which to start grabbing pixels
     * @param Y the y location at which to start grabbing pixels
     * @param W the width of the rectangle of pixels to grab
     * @param H the height of the rectangle of pixels to grab
     * @param pixels a pre-allocated array of pixels of size w*h; can be null
     * @return <code>pixels</code> if non-null, a new array of integers
     *   otherwise
     * @throws IllegalArgumentException is <code>pixels</code> is non-null and
     *   of length &lt; w*h
     */
    public int[] getPixels(final java.awt.image.BufferedImage IMAGE, final int X, final int Y, final int W, final int H, int[] pixels)
    {
        if (W == 0 || H == 0)
        {
            return new int[0];
        }

        if (pixels == null)
        {
            pixels = new int[W * H];
        }
        else if (pixels.length < W * H)
        {
            throw new IllegalArgumentException("pixels array must have a length " + " >= w*h");
        }

        int imageType = IMAGE.getType();
        if (imageType == java.awt.image.BufferedImage.TYPE_INT_ARGB || imageType == java.awt.image.BufferedImage.TYPE_INT_RGB)
        {
            java.awt.image.Raster raster = IMAGE.getRaster();
            return (int[]) raster.getDataElements(X, Y, W, H, pixels);
        }

        // Unmanages the image
        return IMAGE.getRGB(X, Y, W, H, pixels, 0, W);
    }

    /**
     * <p>Writes a rectangular area of pixels in the destination
     * <code>BufferedImage</code>. Calling this method on
     * an image of type different from <code>BufferedImage.TYPE_INT_ARGB</code>
     * and <code>BufferedImage.TYPE_INT_RGB</code> will unmanage the image.</p>
     *
     * @param IMAGE the destination image
     * @param X the x location at which to start storing pixels
     * @param Y the y location at which to start storing pixels
     * @param W the width of the rectangle of pixels to store
     * @param H the height of the rectangle of pixels to store
     * @param pixels an array of pixels, stored as integers
     * @throws IllegalArgumentException is <code>pixels</code> is non-null and
     *   of length &lt; w*h
     */
    public void setPixels(final java.awt.image.BufferedImage IMAGE, final int X, final int Y, final int W, final int H, int[] pixels)
    {
        if (pixels == null || W == 0 || H == 0)
        {
            return;
        }
        else if (pixels.length < W * H)
        {
            throw new IllegalArgumentException("pixels array must have a length" + " >= w*h");
        }

        int imageType = IMAGE.getType();
        if (imageType == java.awt.image.BufferedImage.TYPE_INT_ARGB || imageType == java.awt.image.BufferedImage.TYPE_INT_RGB)
        {
            java.awt.image.WritableRaster raster = IMAGE.getRaster();
            raster.setDataElements(X, Y, W, H, pixels);
        }
        else
        {
            // Unmanages the image
            IMAGE.setRGB(X, Y, W, H, pixels, 0, W);
        }
    }
}