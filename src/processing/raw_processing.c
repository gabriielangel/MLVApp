#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "raw_processing.h"
#include "../mlv/video_mlv.h"

/* Matrix functions which are useful */
#include "../matrix/matrix.h"

#define STANDARD_GAMMA 3.15

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define LIMIT16(X) MAX(MIN(X, 65535), 0)

/* Because why compile a whole .o just for this? */
#include "processing.c"
/* Default image profiles */
#include "image_profiles.c"

/* Initialises processing thing with memory */
processingObject_t * initProcessingObject()
{
    processingObject_t * processing = calloc( 1, sizeof(processingObject_t) );

    processing->pre_calc_curve_r = malloc( 65536 * sizeof(uint16_t) );
    processing->pre_calc_curve_g = malloc( 65536 * sizeof(uint16_t) );
    processing->pre_calc_curve_b = malloc( 65536 * sizeof(uint16_t) );
    processing->pre_calc_gamma   = malloc( 65536 * sizeof(uint16_t) );
    processing->pre_calc_levels  = malloc( 65536 * sizeof(uint16_t) );
    processing->pre_calc_sharp_a = malloc( 65536 * sizeof(uint32_t) );
    processing->pre_calc_sharp_x = malloc( 65536 * sizeof(uint16_t) );
    processing->pre_calc_sharp_y = malloc( 65536 * sizeof(uint16_t) );
    processing->pre_calc_sat     = malloc( 131072 * sizeof(int32_t) );

    /* For precalculated matrix values */
    for (int i = 0; i < 9; ++i)
        processing->pre_calc_matrix[i] = malloc( 65536 * sizeof(int32_t) );

    /* A nothing matrix */
    processing->cam_to_sRGB_matrix[0] = 1.0;
    processing->cam_to_sRGB_matrix[4] = 1.0;
    processing->cam_to_sRGB_matrix[8] = 1.0;
    /* Different matrix BTW */
    processing->xyz_to_rgb_matrix[0] = 1.0;
    processing->xyz_to_rgb_matrix[4] = 1.0;
    processing->xyz_to_rgb_matrix[8] = 1.0;


    double rgb_to_YCbCr[7] = {  0.299000,  0.587000,  0.114000,
                               -0.168736, -0.331264, /* 0.5 */
                               /* 0.5 */  -0.418688, -0.081312 };
    double YCbCr_to_rgb[4] = {             1.402000,
                               -0.344136, -0.714136,
                                1.772000  };

    for (int i = 0; i < 7; ++i)
    {
        processing->cs_zone.pre_calc_rgb_to_YCbCr[i] = malloc( 65536 * sizeof(int32_t) );
        for (int j = 0; j < 65536; ++j)
        {
            processing->cs_zone.pre_calc_rgb_to_YCbCr[i][j] = (double)j * rgb_to_YCbCr[i];
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        processing->cs_zone.pre_calc_YCbCr_to_rgb[i] = malloc( 65536 * sizeof(int32_t) );
        for (int j = 0; j < 65536; ++j)
        {
            processing->cs_zone.pre_calc_YCbCr_to_rgb[i][j] = (double)(j-32768) * YCbCr_to_rgb[i];
        }
    }

    /* Default settings */
    processingSetWhiteBalance(processing, 6250.0, 0.0);
    processingSetBlackAndWhiteLevel(processing, 8192.0, 64000.0); /* 16 bit! */
    processingSetExposureStops(processing, 0.0);
    processingSetGamma(processing, STANDARD_GAMMA);
    processingSetSaturation(processing, 1.0);
    processingSetContrast(processing, 0.73, 5.175, 0.5, 0.0, 0.0);
    processingSetImageProfile(processing, PROFILE_TONEMAPPED);
    processingSetSharpening(processing, 0.0);

    /* Just in case (should be done tho already) */
    processing_update_matrices(processing);
    
    return processing;
}

void processingSetImageProfile(processingObject_t * processing, int imageProfile)
{
    if (imageProfile >= 0 && imageProfile <= 5)
    {
        processingSetCustomImageProfile(processing, &default_image_profiles[imageProfile]);
    }
    else return;
}


/* Image profile strruct needed */
void processingSetCustomImageProfile(processingObject_t * processing, image_profile_t * imageProfile)
{
    processing->image_profile = imageProfile;
    processing->use_rgb_curves = imageProfile->disable_settings.curves;
    processing->use_saturation = imageProfile->disable_settings.saturation;
    processingSetGamma(processing, imageProfile->gamma_power);
    if (imageProfile->disable_settings.tonemapping)
    {
        processing->tone_mapping_function = imageProfile->tone_mapping_function;
        processing_enable_tonemapping(processing);
    }
    else processing_disable_tonemapping(processing);
}


/* Takes those matrices I learned about on the forum */
void processingCamTosRGBMatrix(processingObject_t * processing, double * camTosRGBMatrix)
{
    memcpy(processing->cam_to_sRGB_matrix, camTosRGBMatrix, sizeof(double) * 9);
    /* Calculates final main matrix */
    processing_update_matrices(processing);
}


/* Process a RAW frame with settings from a processing object
 * - image must be debayered and RGB plz + thx! */
void applyProcessingObject( processingObject_t * processing, 
                            int imageX, int imageY, 
                            uint16_t * __restrict inputImage, 
                            uint16_t * __restrict outputImage )
{
    /* Number of elements */
    int img_s = imageX * imageY * 3;

    /* (for shorter code) */
    int32_t ** pm = processing->pre_calc_matrix;
    uint16_t * out_img = outputImage;
    uint16_t * img = inputImage;
    uint16_t * img_end = img + img_s;

    /* Apply some precalcuolated settings */
    for (int i = 0; i < img_s; ++i)
    {
        /* Black + white level */
        img[i] = processing->pre_calc_levels[ img[i] ];
    }

    /* NOW MATRIX! (white balance & exposure) */
    for (uint16_t * pix = img; pix < img_end; pix += 3)
    {
        int32_t pix0 = pm[0][pix[0]] + pm[1][pix[1]] + pm[2][pix[2]];
        int32_t pix1 = pm[3][pix[0]] + pm[4][pix[1]] + pm[5][pix[2]];
        int32_t pix2 = pm[6][pix[0]] + pm[7][pix[1]] + pm[8][pix[2]];

        pix[0] = LIMIT16(pix0);
        pix[1] = LIMIT16(pix1);
        pix[2] = LIMIT16(pix2);
    }

    /* Gamma */
    for (int i = 0; i < img_s; ++i)
    {
        img[i] = processing->pre_calc_gamma[ img[i] ];
    }

    /* Now highlilght reconstruction */
    if (processing->exposure_stops < 0.0 && processing->highest_green < 65535 && processing->highlight_reconstruction)
    {
        for (uint16_t * pix = img; pix < img_end; pix += 3)
        {
            /* Check if its the highest green value possible */
            if (pix[1] == processing->highest_green)
            {
                pix[1] = (pix[0] + pix[2]) / 2;
            }
        }
    }

    if (processing->use_saturation)
    {
        /* Now saturation (looks way better after gamma) */
        for (uint16_t * pix = img; pix < img_end; pix += 3)
        {
            /* Pixel brightness = 4/16 R, 11/16 G, 1/16 blue; Try swapping the channels, it will look worse */
            int32_t Y1 = ((pix[0] << 2) + (pix[1] * 11) + pix[2]) >> 4;
            int32_t Y2 = Y1 - 65536;

            /* Increase difference between channels and the saturation midpoint */
            int32_t pix0 = processing->pre_calc_sat[pix[0] - Y2] + Y1;
            int32_t pix1 = processing->pre_calc_sat[pix[1] - Y2] + Y1;
            int32_t pix2 = processing->pre_calc_sat[pix[2] - Y2] + Y1;

            pix[0] = LIMIT16(pix0);
            pix[1] = LIMIT16(pix1);
            pix[2] = LIMIT16(pix2);
        }
    }

    if (processing->use_rgb_curves)
    {
        /* Contrast Curve (OMG putting this after gamma made it 999x better) */
        for (uint16_t * pix = img; pix < img_end; pix += 3)
        {
            pix[0] = processing->pre_calc_curve_r[ pix[0] ];
            pix[1] = processing->pre_calc_curve_r[ pix[1] ];
            pix[2] = processing->pre_calc_curve_r[ pix[2] ];
        }
    }

    uint32_t sharp_skip = 1; /* Skip how many pixels when applying sharpening */
    uint32_t sharp_start = 0; /* How many pixels offset to start at */

    /* enter YCbCr world - https://en.wikipedia.org/wiki/YCbCr (I used the 'JPEG Transform') */
    if (processingUsesChromaSeparation(processing))
    {
        int32_t ** ry = processing->cs_zone.pre_calc_rgb_to_YCbCr;
        for (uint16_t * pix = img; pix < img_end; pix += 3)
        {
            /* RGB to YCbCr */
            int32_t pix_Y  =         ry[0][pix[0]] + ry[1][pix[1]] + ry[2][pix[2]];
            int32_t pix_Cb = 32768 + ry[3][pix[0]] + ry[4][pix[1]] + (pix[2] >> 1);
            int32_t pix_Cr = 32768 + (pix[0] >> 1) + ry[5][pix[1]] + ry[6][pix[2]];

            pix[0] = LIMIT16(pix_Y);
            pix[1] = LIMIT16(pix_Cb);
            pix[2] = LIMIT16(pix_Cr);
        }

        sharp_start = 0; /* Start at 0 - Luma/Y channel */
        sharp_skip = 3; /* Only sharpen every third (Y/luma) pixel */
    }

    /* Basic box blur */
    if (processingGetChromaBlurRadius(processing) > 0 && processingUsesChromaSeparation(processing))
    {
        memcpy(out_img, img, img_s * sizeof(uint16_t));

        /* Row length */
        int32_t rl = imageX * 3;

        int32_t radius = processingGetChromaBlurRadius(processing);
        int32_t radius_x = radius*3;
        int32_t y_max = imageY + radius;
        int32_t x_max = (imageX + radius);
        int32_t x_lim = rl-3;

        uint32_t blur_diameter = radius*2+1;

        /* Offset - do twice on channel '1' and '2' (Cb and Cr) */
        for (uint32_t offset = 1; offset <=2; ++offset)
        {
            /* Horizontal blur */
            for (int32_t y = 0; y < imageY; ++y) /* rows */
            {
                uint16_t * out_row = out_img + (y * rl); /* current row ouptut */
                uint16_t * row = img + (y * rl); /* current row */

                uint32_t sum = row[offset] * blur_diameter;

                for (int32_t x = -radius; x < imageX; ++x)
                {
                    sum -= row[MIN(MAX(x-radius, 0), imageX-1)*3+offset];
                    sum += row[MIN(MAX(x+radius+1, 0), imageX-1)*3+offset];
                    out_row[MAX(x, 0)*3+offset] = sum / blur_diameter;
                }
            }

            /* Vertical blur */
            for (int32_t x = 0; x < imageX; ++x) /* columns */
            {
                uint16_t * out_col = img + (x*3);
                uint16_t * col = out_img + (x*3);

                uint32_t sum = out_img[x*3+offset] * blur_diameter;

                for (int32_t y = -radius; y < imageY; ++y)
                {
                    sum -= col[MIN(MAX((y-radius), 0), imageY-1)*rl+offset];
                    sum += col[MIN(MAX((y+radius+1), 0), imageY-1)*rl+offset];
                    out_col[MAX(y, 0)*rl+offset] = sum / blur_diameter;
                }
            }
        }

        // memcpy(inputImage, outputImage, img_s * sizeof(uint16_t));
    }

    if (processingGetSharpening(processing) > 0.005)
    {
        /* Avoid gaps in pixels if skipping pixels during sharpen */
        if (sharp_skip != 1) memcpy(outputImage, inputImage, img_s * sizeof(uint16_t));
    
        uint32_t y_max = imageY - 1;
        uint32_t x_max = (imageX - 1) * 3; /* X in multiples of 3 for RGB */

        /* Center and outter lut */
        uint32_t * ka = processing->pre_calc_sharp_a;
        uint16_t * kx = processing->pre_calc_sharp_x;
        uint16_t * ky = processing->pre_calc_sharp_y;
        
        /* Row length elements */
        uint32_t rl = imageX * 3;

        for (uint32_t y = 1; y < y_max; ++y)
        {
            uint16_t * out_row = out_img + (y * rl); /* current row ouptut */
            uint16_t * row = img + (y * rl); /* current row */
            uint16_t * p_row = img + ((y-1) * rl); /* previous */
            uint16_t * n_row = img + ((y+1) * rl); /* next */

            for (uint32_t x = 3+sharp_start; x < x_max; x+=sharp_skip)
            {
                int32_t sharp = ka[row[x]] 
                              - ky[p_row[x]]
                              - ky[n_row[x]]
                              - kx[row[x-3]]
                              - kx[row[x+3]];

                out_row[x] = LIMIT16(sharp);
            }

            /* Edge pixels (basically don't do any changes to them) */
            out_row[0] = row[0];
            out_row[1] = row[1];
            out_row[2] = row[2];
            out_row += rl;
            row += rl;
            out_row[-3] = row[-3];
            out_row[-2] = row[-2];
            out_row[-1] = row[-1];
        }

        /* Copy top and bottom row */
        memcpy(outputImage, inputImage, rl * sizeof(uint16_t));
        memcpy(outputImage + (rl*(imageY-1)), inputImage + (rl*(imageY-1)), rl * sizeof(uint16_t));
    }
    else
    {
        memcpy(outputImage, inputImage, img_s * sizeof(uint16_t));
    }

    /* Leave Y-Cb-Cr world */
    if (processingUsesChromaSeparation(processing))
    {
        img_end = outputImage + img_s;
        int32_t ** yr = processing->cs_zone.pre_calc_YCbCr_to_rgb;
        for (uint16_t * pix = outputImage; pix < img_end; pix += 3)
        {
            int32_t pix_R = pix[0]                 + yr[0][pix[2]];
            int32_t pix_G = pix[0] + yr[1][pix[1]] + yr[2][pix[2]];
            int32_t pix_B = pix[0] + yr[3][pix[1]];

            pix[0] = LIMIT16(pix_R);
            pix[1] = LIMIT16(pix_G);
            pix[2] = LIMIT16(pix_B);
        }
    }
}

/* Set contrast (S-curve really) */
void processingSetContrast( processingObject_t * processing, 
                            double DCRange,  /* Dark contrast range: 0.0 to 1.0 */
                            double DCFactor, /* Dark contrast strength: 0.0 to 8.0(any range really) */
                            double LCRange,  /* Light contrast range */
                            double LCFactor, /* Light contrast strength */
                            double lighten   /* 0-1 (for good highlight rolloff) */ )
{
    /* Basic things */
    processing->light_contrast_factor = LCFactor;
    processing->light_contrast_range = LCRange;
    processing->dark_contrast_factor = DCFactor; 
    processing->dark_contrast_range = DCRange;
    processing->lighten = lighten;

    processing_update_curves(processing);
}

void processingSetDCRange(processingObject_t * processing, double DCRange)
{
    processing->dark_contrast_range = DCRange;
    processing_update_curves(processing);
}
void processingSetDCFactor(processingObject_t * processing, double DCFactor)
{
    processing->dark_contrast_factor = DCFactor;
    processing_update_curves(processing);
}
void processingSetLCRange(processingObject_t * processing, double LCRange) 
{
    processing->light_contrast_range = LCRange;
    processing_update_curves(processing);
}
void processingSetLCFactor(processingObject_t * processing, double LCFactor)
{
    processing->light_contrast_factor = LCFactor;
    processing_update_curves(processing);
}
void processingSetLightening(processingObject_t * processing, double lighten)
{
    processing->lighten = lighten;
    processing_update_curves(processing);
}

/* Have a guess what this does */
void processingSetExposureStops(processingObject_t * processing, double exposureStops)
{
    processing->exposure_stops = exposureStops;

    processingSetGamma(processing, processing->gamma_power);
    processing_update_matrices(processing);
}


/* Sets and precalculaes saturation */
void processingSetSaturation(processingObject_t * processing, double saturationFactor)
{
    processing->saturation = saturationFactor;

    /* Precaluclate for the algorithm */
    for (int i = 0; i < 131072; ++i)
    {
        double value = (i - 65536) * saturationFactor;
        processing->pre_calc_sat[i] = value;
    }
}


/* Set direction bias */
void processingSetSharpeningBias(processingObject_t * processing, double bias)
{
    processing->sharpen_bias = bias;
    /* Recalculates everythin */
    processingSetSharpening(processing, processingGetSharpening(processing));
}


void processingSetSharpening(processingObject_t * processing, double sharpen)
{
    processing->sharpen = sharpen;

    /* Anything more than ~0.5 just looks awful */
    sharpen = pow(sharpen, 1.5) * 0.55;

    double sharpen_x = sharpen * (1.0 - processing->sharpen_bias);
    double sharpen_y = sharpen * (1.0 + processing->sharpen_bias);
    double sharpen_a = 1.0 + (2.0 * sharpen_x) + (2.0 * sharpen_y);

    for (int i = 0; i < 65536; ++i)
    {
        processing->pre_calc_sharp_a[i] = (uint32_t)((double)i * sharpen_a);
        processing->pre_calc_sharp_x[i] = (uint16_t)LIMIT16((double)i * sharpen_x);
        processing->pre_calc_sharp_y[i] = (uint16_t)LIMIT16((double)i * sharpen_y);
    }
}


/* Set white balance by kelvin + tint value */
void processingSetWhiteBalance(processingObject_t * processing, double WBKelvin, double WBTint)
{
    processing->kelvin = WBKelvin;
    processing->wb_tint = WBTint;
    
    /* Kalkulate channel (yes in cone space... soon) multipliers */
    get_kelvin_multipliers_rgb(WBKelvin, processing->wb_multipliers);

    /* Do tint (green and red channel seem to be main ones) */
    processing->wb_multipliers[2] += (WBTint / 11.0);
    processing->wb_multipliers[0] += (WBTint / 19.0);

    /* Make all channel multipliers be >= 1 */
    double lowest = MIN( MIN( processing->wb_multipliers[0], 
                              processing->wb_multipliers[1] ), 
                              processing->wb_multipliers[2] );

    for (int i = 0; i < 3; ++i) processing->wb_multipliers[i] /= lowest;

    /* White balance is part of the matrix */
    processing_update_matrices(processing);
}

/* WB just by kelvin */
void processingSetWhiteBalanceKelvin(processingObject_t * processing, double WBKelvin)
{
    processingSetWhiteBalance( processing, WBKelvin,
                               processingGetWhiteBalanceTint(processing) );
}

/* WB tint */
void processingSetWhiteBalanceTint(processingObject_t * processing, double WBTint)
{
    processingSetWhiteBalance( processing, processingGetWhiteBalanceKelvin(processing),
                               WBTint );
}


/* Set gamma (Log-ing / tonemapping done here) */
void processingSetGamma(processingObject_t * processing, double gammaValue)
{
    processing->gamma_power = gammaValue;

    /* Needs to be inverse */
    double gamma = 1.0 / gammaValue;

    if (processing->exposure_stops < 0.0 || !processing->tone_mapping)
    {
        /* Precalculate the exposure curve */
        for (int i = 0; i < 65536; ++i)
        {
            /* Tone mapping also (reinhard) */
            double pixel = (double)i/65535.0;
            if (processing->tone_mapping) pixel = processing->tone_mapping_function(pixel);
            processing->pre_calc_gamma[i] = (uint16_t)(65535.0 * pow(pixel, gamma));
        }
    }
    /* Else exposure done here if it is positive */
    else
    {
        /* Exposure */
        double exposure_factor = pow(2.0, processing->exposure_stops);
        /* Precalculate the curve */
        for (int i = 0; i < 65536; ++i)
        {
            /* Tone mapping also (reinhard) */
            double pixel = (double)i/65535.0;
            pixel *= exposure_factor;
            pixel = processing->tone_mapping_function(pixel);
            pixel = 65535.0 * pow(pixel, gamma);
            pixel = LIMIT16(pixel);
            processing->pre_calc_gamma[i] = pixel;
        }
    }
    /* So highlight reconstruction works */
    processing_update_highest_green(processing);
}

/* Range of saturation and hue is 0.0-1.0 */
void processingSet3WayCorrection( processingObject_t * processing,
                                  double highlightHue, double highlightSaturation,
                                  double midtoneHue, double midtoneSaturation,
                                  double shadowHue, double shadowSaturation )
{
    processing->highlight_hue = highlightHue;
    processing->highlight_sat = highlightSaturation;
    processing->midtone_hue = midtoneHue;
    processing->midtone_sat = midtoneSaturation;
    processing->shadow_hue = shadowHue;
    processing->shadow_sat = shadowSaturation;

    processing_update_curves(processing);
}

void processing_enable_tonemapping(processingObject_t * processing)
{
    (processing)->tone_mapping = 1;
    /* This will update everything necessary to enable tonemapping */
    processingSetGamma(processing, processing->gamma_power);
    processing_update_matrices(processing);
}

void processing_disable_tonemapping(processingObject_t * processing) 
{
    (processing)->tone_mapping = 0;
    /* This will update everything necessary to disable tonemapping */
    processingSetGamma(processing, processing->gamma_power);
    processing_update_matrices(processing);
}

/* Set black and white level */
void processingSetBlackAndWhiteLevel( processingObject_t * processing, 
                                      int blackLevel, int whiteLevel )
{
    processing->black_level = blackLevel;
    processing->white_level = whiteLevel;

    /* How much it needs to be stretched */
    double stretch = 65535.0 / (whiteLevel - blackLevel);

    for (int i = 0; i < 65536; ++i)
    {
        /* Stretch to the black-white level range */
        int new_value = (int)((double)(i - blackLevel) * stretch);

        if (new_value < 65536 && new_value > 0)
        {
            processing->pre_calc_levels[i] = new_value;
        }
        else if (new_value < 0)
        {
            processing->pre_calc_levels[i] = 0;
        }
        else if (new_value > 65535)
        {
            processing->pre_calc_levels[i] = 65535;
        }
    }
}

/* Cheat functions */
void processingSetBlackLevel(processingObject_t * processing, int blackLevel)
{
    processingSetBlackAndWhiteLevel( processing,
                                     blackLevel,
                                     processingGetWhiteLevel(processing) );
}
void processingSetWhiteLevel(processingObject_t * processing, int whiteLevel)
{
    processingSetBlackAndWhiteLevel( processing,
                                     processingGetBlackLevel(processing),
                                     whiteLevel );
}

/* Decomissions a processing object completely(I hope) */
void freeProcessingObject(processingObject_t * processing)
{
    free(processing->pre_calc_curve_r);
    free(processing->pre_calc_curve_g);
    free(processing->pre_calc_curve_b);
    free(processing->pre_calc_gamma);
    free(processing->pre_calc_levels);
    free(processing->pre_calc_sat);
    free(processing->pre_calc_sharp_a);
    free(processing->pre_calc_sharp_x);
    free(processing->pre_calc_sharp_y);
    for (int i = 8; i >= 0; --i) free(processing->pre_calc_matrix[i]);
    for (int i = 6; i >= 0; --i) free(processing->cs_zone.pre_calc_rgb_to_YCbCr[i]);
    for (int i = 3; i >= 0; --i) free(processing->cs_zone.pre_calc_YCbCr_to_rgb[i]);
    free(processing);
}
