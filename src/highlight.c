/* 
 * Copyright (C) 2000 Rich Wareham <richwareham@users.sourceforge.net>
 * 
 * This file is part of libdvdnav, a DVD navigation library.
 * 
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#define BUTTON_TESTING

#include <assert.h>

#include <dvdnav.h>
#include "dvdnav_internal.h"

#include "vm.h"
#include <dvdread/nav_types.h>

#ifdef BUTTON_TESTING
#include <dvdread/nav_print.h>
#include "vmcmd.h"

static void print_time(dvd_time_t *dtime) {
  const char *rate;
  assert((dtime->hour>>4) < 0xa && (dtime->hour&0xf) < 0xa);
  assert((dtime->minute>>4) < 0x7 && (dtime->minute&0xf) < 0xa);
  assert((dtime->second>>4) < 0x7 && (dtime->second&0xf) < 0xa);
  assert((dtime->frame_u&0xf) < 0xa);

  fprintf(MSG_OUT,"%02x:%02x:%02x.%02x",
         dtime->hour,
         dtime->minute,
         dtime->second,
         dtime->frame_u & 0x3f);
  switch((dtime->frame_u & 0xc0) >> 6) {
  case 1:
    rate = "25.00";
    break;
  case 3:
    rate = "29.97";
    break;
  default:
    rate = "(please send a bug report)";
    break;
  }
  fprintf(MSG_OUT," @ %s fps", rate);
}

static void nav_print_PCI_GI(pci_gi_t *pci_gi) {
  int i;

  fprintf(MSG_OUT,"pci_gi:\n");
  fprintf(MSG_OUT,"nv_pck_lbn    0x%08x\n", pci_gi->nv_pck_lbn);
  fprintf(MSG_OUT,"vobu_cat      0x%04x\n", pci_gi->vobu_cat);
  fprintf(MSG_OUT,"vobu_uop_ctl  0x%08x\n", *(uint32_t*)&pci_gi->vobu_uop_ctl);
  fprintf(MSG_OUT,"vobu_s_ptm    0x%08x\n", pci_gi->vobu_s_ptm);
  fprintf(MSG_OUT,"vobu_e_ptm    0x%08x\n", pci_gi->vobu_e_ptm);
  fprintf(MSG_OUT,"vobu_se_e_ptm 0x%08x\n", pci_gi->vobu_se_e_ptm);
  fprintf(MSG_OUT,"e_eltm        ");
  print_time(&pci_gi->e_eltm);
  fprintf(MSG_OUT,"\n");

  fprintf(MSG_OUT,"vobu_isrc     \"");
  for(i = 0; i < 32; i++) {
    char c = pci_gi->vobu_isrc[i];
    if((c >= ' ') && (c <= '~'))
      fprintf(MSG_OUT,"%c", c);
    else
      fprintf(MSG_OUT,".");
  }
  fprintf(MSG_OUT,"\"\n");
}

static void nav_print_NSML_AGLI(nsml_agli_t *nsml_agli) {
  int i, j = 0;

  for(i = 0; i < 9; i++)
    j |= nsml_agli->nsml_agl_dsta[i];
  if(j == 0)
    return;

  fprintf(MSG_OUT,"nsml_agli:\n");
  for(i = 0; i < 9; i++)
    if(nsml_agli->nsml_agl_dsta[i])
      fprintf(MSG_OUT,"nsml_agl_c%d_dsta  0x%08x\n", i + 1,
             nsml_agli->nsml_agl_dsta[i]);
}

static void nav_print_HL_GI(hl_gi_t *hl_gi, int *btngr_ns, int *btn_ns) {

  if((hl_gi->hli_ss & 0x03) == 0)
    return;

  fprintf(MSG_OUT,"hl_gi:\n");
  fprintf(MSG_OUT,"hli_ss        0x%01x\n", hl_gi->hli_ss & 0x03);
  fprintf(MSG_OUT,"hli_s_ptm     0x%08x\n", hl_gi->hli_s_ptm);
  fprintf(MSG_OUT,"hli_e_ptm     0x%08x\n", hl_gi->hli_e_ptm);
  fprintf(MSG_OUT,"btn_se_e_ptm  0x%08x\n", hl_gi->btn_se_e_ptm);

  *btngr_ns = hl_gi->btngr_ns;
  fprintf(MSG_OUT,"btngr_ns      %d\n",  hl_gi->btngr_ns);
  fprintf(MSG_OUT,"btngr%d_dsp_ty    0x%02x\n", 1, hl_gi->btngr1_dsp_ty);
  fprintf(MSG_OUT,"btngr%d_dsp_ty    0x%02x\n", 2, hl_gi->btngr2_dsp_ty);
  fprintf(MSG_OUT,"btngr%d_dsp_ty    0x%02x\n", 3, hl_gi->btngr3_dsp_ty);

  fprintf(MSG_OUT,"btn_ofn       %d\n", hl_gi->btn_ofn);
  *btn_ns = hl_gi->btn_ns;
  fprintf(MSG_OUT,"btn_ns        %d\n", hl_gi->btn_ns);
  fprintf(MSG_OUT,"nsl_btn_ns    %d\n", hl_gi->nsl_btn_ns);
  fprintf(MSG_OUT,"fosl_btnn     %d\n", hl_gi->fosl_btnn);
  fprintf(MSG_OUT,"foac_btnn     %d\n", hl_gi->foac_btnn);
}

static void nav_print_BTN_COLIT(btn_colit_t *btn_colit) {
  int i, j;

  j = 0;
  for(i = 0; i < 6; i++)
    j |= btn_colit->btn_coli[i/2][i&1];
  if(j == 0)
    return;

  fprintf(MSG_OUT,"btn_colit:\n");
  for(i = 0; i < 3; i++)
    for(j = 0; j < 2; j++)
      fprintf(MSG_OUT,"btn_cqoli %d  %s_coli:  %08x\n",
             i, (j == 0) ? "sl" : "ac",
             btn_colit->btn_coli[i][j]);
}

static void nav_print_BTNIT(btni_t *btni_table, int btngr_ns, int btn_ns) {
  int i, j, k;

  fprintf(MSG_OUT,"btnit:\n");
  fprintf(MSG_OUT,"btngr_ns: %i\n", btngr_ns);
  fprintf(MSG_OUT,"btn_ns: %i\n", btn_ns);

  if(btngr_ns == 0)
    return;

  for(i = 0; i < btngr_ns; i++) {
    for(j = 0; j < (36 / btngr_ns); j++) {
      if(j < btn_ns) {
        btni_t *btni = &btni_table[(36 / btngr_ns) * i + j];

        fprintf(MSG_OUT,"group %d btni %d:  ", i+1, j+1);
        fprintf(MSG_OUT,"btn_coln %d, auto_action_mode %d\n",
               btni->btn_coln, btni->auto_action_mode);
        fprintf(MSG_OUT,"coords   (%d, %d) .. (%d, %d)\n",
               btni->x_start, btni->y_start, btni->x_end, btni->y_end);

        fprintf(MSG_OUT,"up %d, ", btni->up);
        fprintf(MSG_OUT,"down %d, ", btni->down);
        fprintf(MSG_OUT,"left %d, ", btni->left);
        fprintf(MSG_OUT,"right %d\n", btni->right);
        for(k = 0; k < 8; k++) {
          fprintf(MSG_OUT, "%02x ", btni->cmd.bytes[k]);
        }
        fprintf(MSG_OUT, "| ");
        vmPrint_mnemonic(&btni->cmd);
        fprintf(MSG_OUT, "\n\n");
      }
    }
  }
}

static void nav_print_HLI(hli_t *hli) {
  int btngr_ns = 0, btn_ns = 0;

  fprintf(MSG_OUT,"hli:\n");
  nav_print_HL_GI(&hli->hl_gi, & btngr_ns, & btn_ns);
  nav_print_BTN_COLIT(&hli->btn_colit);
  nav_print_BTNIT(hli->btnit, btngr_ns, btn_ns);
}

void nav_print_PCI(pci_t *pci) {
  fprintf(MSG_OUT,"pci packet:\n");
  nav_print_PCI_GI(&pci->pci_gi);
  nav_print_NSML_AGLI(&pci->nsml_agli);
  nav_print_HLI(&pci->hli);
}


#endif

/* Highlighting API calls */



dvdnav_status_t dvdnav_get_current_highlight(dvdnav_t *this, int* button) {
  if(!this)
   return S_ERR;

  /* Simply return the appropriate value based on the SPRM */
  (*button) = (this->vm->state.HL_BTNN_REG) >> 10;
  
  return S_OK;
}

btni_t *__get_current_button(dvdnav_t *this) {
  int button = 0;

  if(dvdnav_get_current_highlight(this, &button) != S_OK) {
    printerr("Unable to get information on current highlight.");
    return NULL;
  }
#ifdef BUTTON_TESTING
  nav_print_PCI(&(this->pci));
#endif
  
  return &(this->pci.hli.btnit[button-1]);
}

dvdnav_status_t dvdnav_button_auto_action(dvdnav_t *this) {
  btni_t *button_ptr;
  
  if(!this)
   return S_ERR;

  if((button_ptr = __get_current_button(this)) == NULL) {
    return S_ERR;
  }
  if (button_ptr->auto_action_mode == 1) {
    return S_OK;
  }
  return S_ERR;
}


dvdnav_status_t dvdnav_upper_button_select(dvdnav_t *this) {
  btni_t *button_ptr;
  
  if(!this)
   return S_ERR;

  if((button_ptr = __get_current_button(this)) == NULL) {
    return S_ERR;
  }

  dvdnav_button_select(this, button_ptr->up);
  if (dvdnav_button_auto_action(this) ) {
    dvdnav_button_activate(this);
  }
 
  return S_OK;
}

dvdnav_status_t dvdnav_lower_button_select(dvdnav_t *this) {
  btni_t *button_ptr;
  
  if(!this)
   return S_ERR;

  if((button_ptr = __get_current_button(this)) == NULL) {
    return S_ERR;
  }

  dvdnav_button_select(this, button_ptr->down);
  if (dvdnav_button_auto_action(this) ) {
    dvdnav_button_activate(this);
  }
  
  return S_OK;
}

dvdnav_status_t dvdnav_right_button_select(dvdnav_t *this) {
  btni_t *button_ptr;
  
  if(!this)
   return S_ERR;

  if((button_ptr = __get_current_button(this)) == NULL) {
    printerr("Error fetching information on current button.");
    return S_ERR;
  }

  dvdnav_button_select(this, button_ptr->right);
  if (dvdnav_button_auto_action(this) ) {
    dvdnav_button_activate(this);
  }
  
  return S_OK;
}

dvdnav_status_t dvdnav_left_button_select(dvdnav_t *this) {
  btni_t *button_ptr;
  
  if(!this)
   return S_ERR;

  if((button_ptr = __get_current_button(this)) == NULL) {
    return S_ERR;
  }

  dvdnav_button_select(this, button_ptr->left);
  if (dvdnav_button_auto_action(this) ) {
    dvdnav_button_activate(this);
  }
  
  return S_OK;
}

dvdnav_status_t dvdnav_get_highlight_area(pci_t* nav_pci , int32_t button, int32_t mode, 
                                           dvdnav_highlight_area_t* highlight) {
  btni_t *button_ptr;
#ifdef BUTTON_TESTING
  fprintf(MSG_OUT, "libdvdnav: Button get_highlight_area %i\n", button);
#endif

  /* Set the highlight SPRM if the passed button was valid*/
  if((button <= 0) || (button > nav_pci->hli.hl_gi.btn_ns)) {
    fprintf(MSG_OUT, "libdvdnav: Unable to select button number %i as it doesn't exist\n",
              button);
    return S_ERR;
  }
  button_ptr = &nav_pci->hli.btnit[button-1];

  highlight->sx = button_ptr->x_start;
  highlight->sy = button_ptr->y_start;
  highlight->ex = button_ptr->x_end;
  highlight->ey = button_ptr->y_end;
  if(button_ptr->btn_coln != 0) {
    highlight->palette = nav_pci->hli.btn_colit.btn_coli[button_ptr->btn_coln-1][mode];
  } else {
    highlight->palette = 0;
  }
  highlight->pts = nav_pci->hli.hl_gi.hli_s_ptm;
  highlight->buttonN = button;
#ifdef BUTTON_TESTING
  fprintf(MSG_OUT, "libdvdnav: highlight.c:Highlight area is (%u,%u)-(%u,%u), display = %i, button = %u\n",
               button_ptr->x_start, button_ptr->y_start,
               button_ptr->x_end, button_ptr->y_end,
               1,
               button);
#endif

  return S_OK;
}

dvdnav_status_t dvdnav_button_activate(dvdnav_t *this) {
  int button;
  btni_t *button_ptr = NULL;
  
  if(!this) 
   return S_ERR;
  pthread_mutex_lock(&this->vm_lock); 

  /* Precisely the same as selecting a button except we want
   * a different palette */
  if(dvdnav_get_current_highlight(this, &button) != S_OK) {
    pthread_mutex_unlock(&this->vm_lock); 
    return S_ERR;
  }
/* FIXME: dvdnav_button_select should really return a
 * special case for explicit NO-BUTTONS.
 */
  if(dvdnav_button_select(this, button) != S_OK) {
    /* Special code to handle still menus with no buttons.
     * the navigation is expected to report to the appicatino that a STILL is
     * underway. In turn, the application is supposed to report to the user
     * that the playback is pause. The user is then expected to undo the pause.
     * ie: hit play. At that point, the navigation should release the still and
     * go to the next Cell.
     * Explanation by Mathieu Lavage <mathieu_lacage@realmagic.fr>
     * Code added by jcdutton.
     */
    if (this->position_current.still != 0) {
      /* In still, but no buttons. */
      vm_get_next_cell(this->vm);
      this->position_current.still = 0;
      pthread_mutex_unlock(&this->vm_lock);
      return S_OK;
    }
    pthread_mutex_unlock(&this->vm_lock); 
    return S_ERR;
  }
  /* FIXME: The button command should really be passed in the API instead. */ 
  button_ptr = __get_current_button(this);
  /* Finally, make the VM execute the appropriate code and
   * scedule a jump */
#ifdef BUTTON_TESTING
  fprintf(MSG_OUT, "libdvdnav: Evaluating Button Activation commands.\n");
#endif
  if(vm_eval_cmd(this->vm, &(button_ptr->cmd)) == 1) {
    /* Command caused a jump */
    this->vm->hop_channel++;
    this->position_current.still = 0;
  }
  pthread_mutex_unlock(&this->vm_lock); 
  return S_OK;
}

dvdnav_status_t dvdnav_button_select(dvdnav_t *this, int button) {
  
  if(!this) {
   printerrf("Unable to select button number %i as this state bad",
	      button);
   return S_ERR;
  }
 
#ifdef BUTTON_TESTING
  fprintf(MSG_OUT, "libdvdnav: Button select %i\n", button); 
#endif
  
  /* Set the highlight SPRM if the passed button was valid*/
  /* FIXME: this->pci should be provided by the application. */
  if((button <= 0) || (button > this->pci.hli.hl_gi.btn_ns)) {
    printerrf("Unable to select button number %i as it doesn't exist",
	      button);
    return S_ERR;
  }
  this->vm->state.HL_BTNN_REG = (button << 10);

  this->hli_state = 1; /* Selected */

  this->position_current.button = -1; /* Force Highligh change */

  return S_OK;
}

dvdnav_status_t dvdnav_button_select_and_activate(dvdnav_t *this, 
						  int button) {
  /* A trivial function */
  if(dvdnav_button_select(this, button) != S_ERR) {
    return dvdnav_button_activate(this);
  }
  
  /* Should never get here without an error */
  return S_ERR;
}

dvdnav_status_t dvdnav_mouse_select(dvdnav_t *this, int x, int y) {
  int button, cur_button;
  uint32_t best,dist;
  int mx,my,dx,dy,d;

  /* FIXME: At the moment, the case of no button matchin (x,y) is
   * silently ignored, is this OK? */
  if(!this)
   return S_ERR;

  if(dvdnav_get_current_highlight(this, &cur_button) != S_OK) {
    return S_ERR;
  }

  best = 0; 
  dist = 0x08000000; /* >> than  (720*720)+(567*567); */
  
  /* Loop through each button */
  for(button=1; button <= this->pci.hli.hl_gi.btn_ns; button++) {
    btni_t *button_ptr = NULL;
    button_ptr = &(this->pci.hli.btnit[button-1]);
    if((x >= button_ptr->x_start) && (x <= button_ptr->x_end) &&
       (y >= button_ptr->y_start) && (y <= button_ptr->y_end)) {
      mx = (button_ptr->x_start + button_ptr->x_end)/2;
	  my = (button_ptr->y_start + button_ptr->y_end)/2;
      dx = mx - x;
      dy = my - y;
      d = (dx*dx) + (dy*dy);
      /* If the mouse is within the button and the mouse is closer
       * to the center of this button then it is the best choice. */
      if(d < dist) {
        dist = d; best=button;
      }
    }
  }
			  
  if (best!=0) {
    /* As an efficiency measure, only re-select the button
     * if it is different to the previously selected one. */
    if(best != cur_button) {
      dvdnav_button_select(this, best);
    }
  }
  
  return S_OK;
}

dvdnav_status_t dvdnav_mouse_activate(dvdnav_t *this, int x, int y) {
  /* A trivial function */
  if(dvdnav_mouse_select(this, x,y) != S_ERR) {
    return dvdnav_button_activate(this);
  }
  
  /* Should never get here without an error */
  return S_ERR;
}

