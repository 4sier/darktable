/*
    This file is part of darktable,
    copyright (c) 2011 henrik andersson.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "control/control.h"
#include "blend.h"

#define CLIP_MAX(x) (fmax(0,fmin(max,x)))

typedef float (_blend_func)(float max,float a,float b);

/* normal blend */
static float _blend_normal(float max,float a,float b)
{
  return b;
}
/* lighten */
static float _blend_lighten(float max, float a,float b)
{
  return fmax(a,b);
}
/* darken */
static float _blend_darken(float max, float a,float b)
{
  return fmin(a,b);
}
/* multiply */
static float _blend_multiply(float max, float a,float b)
{
  return (a*b);
}
/* average */
static float _blend_average(float max, float a,float b)
{
  return (a+b)/2.0;
}
/* add */
static float _blend_add(float max, float a,float b)
{
  return CLIP_MAX(a+b);
}
/* substract */
static float _blend_substract(float max, float a,float b)
{
  return ((a+b<max) ? 0:(b+a-max));
}
/* difference */
static float _blend_difference(float max, float a,float b)
{
  return fabs(a-b);
}

/* screen */
static float _blend_screen(float max, float a,float b)
{
  return max - (max-a) * (max-b);
}

/* overlay */
static float _blend_overlay(float max, float a,float b)
{
  const float halfmax=max/2.0;
  const float doublemax=max*2.0;
  return (a>halfmax) ? max - (max - doublemax*(a-halfmax)) * (max-b) : (doublemax*a) * b;
}

/* softlight */
static float _blend_softlight(float max, float a,float b)
{
  const float halfmax=max/2.0;
  return (a>halfmax) ? max - (max-a) * (max - (b-halfmax)) : a * (b+halfmax);
}

/* hardlight */
static float _blend_hardlight(float max, float a,float b)
{
  const float halfmax=max/2.0;
  const float doublemax=max*2.0;
  return (a>halfmax) ? max - (max-a) * (max - doublemax*(b-halfmax)) : a * (b+halfmax);
}

/* vividlight */
static float _blend_vividlight(float max, float a,float b)
{
  const float halfmax=max/2.0;
  const float doublemax=max*2.0;
  return (a>halfmax) ? max - (max-a) / (doublemax*(b-halfmax)) : a / (max-doublemax*b);
}

/* linearlight */
static float _blend_linearlight(float max, float a,float b)
{
  const float halfmax=max/2.0;
  const float doublemax=max*2.0;
  return (a>halfmax) ? a + doublemax*(b-halfmax) : a +doublemax*b-max;
}

/* pinlight */
static float _blend_pinlight(float max, float a,float b)
{
  const float halfmax=max/2.0;
  const float doublemax=max*2.0;
  return (a>halfmax) ? fmax(a,doublemax*(b-halfmax)) : fmin(a,doublemax*b);
}

void dt_develop_blend_process (struct dt_iop_module_t *self, struct dt_dev_pixelpipe_iop_t *piece, void *i, void *o, const struct dt_iop_roi_t *roi_in, const struct dt_iop_roi_t *roi_out)
{
  float *in =(float *)i;
  float *out =(float *)o;
  const int ch = piece->colors;
  _blend_func *blend = NULL;
  dt_develop_blend_params_t *d = (dt_develop_blend_params_t *)piece->blendop_data;
  /* check if blend is disabled */
  if (!d || d->mode==0) return;

  /* select the blend operator */
  const int mode = (d->mode&=~DEVELOP_BLEND_MASK_FLAG);
  switch (mode)
  {
    case DEVELOP_BLEND_LIGHTEN:
      blend = _blend_lighten;
      break;

    case DEVELOP_BLEND_DARKEN:
      blend = _blend_darken;
      break;

    case DEVELOP_BLEND_MULTIPLY:
      blend = _blend_multiply;
      break;

    case DEVELOP_BLEND_AVERAGE:
      blend = _blend_average;
      break;
    case DEVELOP_BLEND_ADD:
      blend = _blend_add;
      break;
    case DEVELOP_BLEND_SUBSTRACT:
      blend = _blend_substract;
      break;
    case DEVELOP_BLEND_DIFFERENCE:
      blend = _blend_difference;
      break;
    case DEVELOP_BLEND_SCREEN:
      blend = _blend_screen;
      break;
    case DEVELOP_BLEND_OVERLAY:
      blend = _blend_overlay;
      break;
    case DEVELOP_BLEND_SOFTLIGHT:
      blend = _blend_softlight;
      break;
    case DEVELOP_BLEND_HARDLIGHT:
      blend = _blend_hardlight;
      break;
    case DEVELOP_BLEND_VIVIDLIGHT:
      blend = _blend_vividlight;
      break;
    case DEVELOP_BLEND_LINEARLIGHT:
      blend = _blend_linearlight;
      break;
    case DEVELOP_BLEND_PINLIGHT:
      blend = _blend_pinlight;
      break;

      /* fallback to normal blend */
    case DEVELOP_BLEND_NORMAL:
    default:
      blend = _blend_normal;
      break;
  }

  if (d->mode)
  {
    
    const gboolean use_mask = (d->mode & DEVELOP_BLEND_MASK_FLAG)?TRUE:FALSE;
    /* defaults to 3 cannels */
    int channels = 3;

    /* get the clipped opacity value  0 - 1 */
    const float opacity = fmin(fmax(0,(d->opacity/100.0)),1.0);

    /* get channel max values depending on colorspace */
    float max[4] = {1.0,1.0,1.0,1.0};
    const dt_iop_colorspace_type_t cst = dt_iop_module_colorspace(self);

    /* if module is in lab space, lets set max for L */
    if (cst == iop_cs_Lab)
    {
      max[0] = 100.0;
      channels = 1;	// in Lab space, only blend Lightness
    }
    else if(cst == iop_cs_RAW)
      channels = 1;	// R G1 B G2
#ifdef _OPENMP
    #pragma omp parallel for default(none) shared(in,roi_out,out,blend,d,max,channels)
#endif
    for (int y=0; y<roi_out->height; y++)
      for (int x=0; x<roi_out->width; x++)
      {
        
        int index=(y*roi_out->width+x);
        float local_opacity = use_mask ? opacity * out[4*index+3] : opacity;
        if (cst == iop_cs_Lab)
        {
          index*=ch;
          /* if in Lab space, just blend Lightness and copy ab from source */
          out[index] = (in[index] * (1.0-local_opacity)) + (blend(max[0], in[index], out[index]) * local_opacity);
          out[index+1] = in[index+1];
          out[index+2] = in[index+2];
        }
        else if (cst == iop_cs_RAW)
        {
          /* handle blend of raw data */
          out[index] = (in[index] * (1.0-local_opacity)) + (blend(max[0], in[index], out[index]) * local_opacity);
        }
        else
        {
          index*=ch;
          /* else assume raw,rgb, and blend each channel */
          for (int k=index; k<(index+channels); k++)
            out[k] = (in[k] * (1.0-local_opacity)) + (blend(max[k-index], in[k], out[k]) * local_opacity);
        }
      }
  }
  else
  {
    /* blending with mask */
    dt_control_log("blending using masks is not yet implemented.");

  }
}