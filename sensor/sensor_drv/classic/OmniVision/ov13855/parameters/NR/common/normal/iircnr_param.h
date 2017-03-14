/*param0.*/
/*v21_sensor_iircnr_level*/
{
    /*cnr_iir_mode*/
    0x0660,
        /*cnr_uv_pg_th*/
        0x04C8,
        /*ymd_u*/
        0x04C80660,
        /*ymd_v*/
        0x00000066,
        /*ymd_min_u*/
        0x5F3F1300,
        /*ymd_min_v*/
        0xDFBF9F7F,
        /*slop_uv*/
        {
            0x1010, 0x1010, 0x1010, 0x1010, 0x3A32,
            0x0703, 0x0703, 0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF,
            0x03FF, 0x03FF, 0x03FF /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x0000005C, 0x00000074, 0x0000005C, 0x40404040, 0x40404040,
            0xFFFF1002, 0xFFFFFFFF, 0x05050505 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x05050505, 0x5F3F1905, 0xDFBF9F7F, 0x00000000, 0x00000000,
            0x00000000, 0x080F0100, 0x00000004 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0100,
            /*iircnr_sat_ratio*/
            0x080F,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0004, 0x0000, 0x0201, 0x080F, 0x0004, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x03FF03FF,

        },
        /*bypass*/
        0x0000005C,
}
,
    /*param1.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0074,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x0000005C,
        /*ymd_v*/
        0x40404040,
        /*ymd_min_u*/
        0x40404040,
        /*ymd_min_v*/
        0xFFFF1002,
        /*slop_uv*/
        {
            0xFFFF, 0xFFFF, 0x0505, 0x0505, 0x0505, 0x0505, 0x1905,
            0x5F3F /*0-7*/
        },
        /*slop_y*/
        {
            0x9F7F, 0xDFBF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x080F0100, 0x00000004, 0x080F0100, 0x00000004, 0x080F0201,
            0x00000004, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0660, 0x04C8,
             0x0660 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x66,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x1300, 0x5F3F, 0x9F7F, 0xDFBF, 0x1010, 0x1010, 0x1010,
             0x1010 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x3A32,
         /*iircnr_alpha_hl_diff_v*/
         0x0703,
         /*iircnr_alpha_low_u*/
         0x00000703,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x080F0100,
    },
    /*param2.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0004,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x080F0100,
        /*ymd_v*/
        0x00000004,
        /*ymd_min_u*/
        0x080F0201,
        /*ymd_min_v*/
        0x00000004,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x04C80660, 0x04C80660,
            0x00000066, 0x5F3F1300, 0xDFBF9F7F /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x1010,
            /*iircnr_sat_ratio*/
            0x1010,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x1010, 0x1010, 0x3A32, 0x0703, 0x0703, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x00FF,
         /*iircnr_uv_th*/
         0x00FF,
         /*iircnr_uv_dist*/
         0x00FF,
         /*uv_low_thr1*/
         {
             0x03FF, 0x03FF, 0x03FF, 0x005C, 0x0000, 0x0074, 0x0000,
             0x005C /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x40,
         /*iircnr_y_th*/
         0x40,
         /*iircnr_y_min_th*/
         0x40,
         /*iircnr_uv_diff_thr*/
         0x40,
         /*y_edge_thr_min*/
         {
             0x4040, 0x4040, 0x1002, 0xFFFF, 0xFFFF, 0xFFFF, 0x0505,
             0x0505 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0505,
         /*iircnr_alpha_hl_diff_v*/
         0x0505,
         /*iircnr_alpha_low_u*/
         0x5F3F1905,
         /*iircnr_alpha_low_v*/
         0xDFBF9F7F,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param3.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0660, 0x04C8, 0x0660, 0x04C8, 0x0066,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x5F3F1300, 0xDFBF9F7F, 0x10101010, 0x10101010, 0x07033A32,
            0x00000703, 0x00000000, 0x00FF00FF /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x03FF00FF, 0x03FF03FF, 0x0000005C, 0x00000074, 0x0000005C,
            0x40404040, 0x40404040, 0xFFFF1002 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0xFFFF,
            /*iircnr_sat_ratio*/
            0xFFFF,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0505, 0x0505, 0x0505, 0x0505, 0x1905, 0x5F3F, 0x9F7F,
             0xDFBF /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0100, 0x080F, 0x0004, 0x0000,
             0x0100 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x04,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0201, 0x080F, 0x0004, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000066,

        },
        /*bypass*/
        0x5F3F1300,
    },
    /*param4.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x9F7F,
        /*cnr_uv_pg_th*/
        0xDFBF,
        /*ymd_u*/
        0x10101010,
        /*ymd_v*/
        0x10101010,
        /*ymd_min_u*/
        0x07033A32,
        /*ymd_min_v*/
        0x00000703,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x03FF, 0x03FF,
            0x03FF /*0-7*/
        },
        /*slop_y*/
        {
            0x005C, 0x0000, 0x0074, 0x0000, 0x005C, 0x0000, 0x4040,
            0x4040 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x40404040, 0xFFFF1002, 0xFFFFFFFF, 0x05050505, 0x05050505,
            0x5F3F1905, 0xDFBF9F7F, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x080F0100, 0x00000004, 0x080F0100,
            0x00000004, 0x080F0201, 0x00000004 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x40404040,

        },
        /*bypass*/
        0x40404040,
    },
    /*param5.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x1002,
        /*cnr_uv_pg_th*/
        0xFFFF,
        /*ymd_u*/
        0xFFFFFFFF,
        /*ymd_v*/
        0x05050505,
        /*ymd_min_u*/
        0x05050505,
        /*ymd_min_v*/
        0x5F3F1905,
        /*slop_uv*/
        {
            0x9F7F, 0xDFBF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0100, 0x080F, 0x0004, 0x0000, 0x0100, 0x080F, 0x0004,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x080F0201, 0x00000004, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0660,
         /*iircnr_uv_th*/
         0x04C8,
         /*iircnr_uv_dist*/
         0x0660,
         /*uv_low_thr1*/
         {
             0x04C8, 0x0066, 0x0000, 0x1300, 0x5F3F, 0x9F7F, 0xDFBF,
             0x1010 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x10,
         /*iircnr_y_th*/
         0x10,
         /*iircnr_y_min_th*/
         0x10,
         /*iircnr_uv_diff_thr*/
         0x10,
         /*y_edge_thr_min*/
         {
             0x3A32, 0x0703, 0x0703, 0x0000, 0x0000, 0x0000, 0x00FF,
             0x00FF /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x00FF,
         /*iircnr_alpha_hl_diff_v*/
         0x03FF,
         /*iircnr_alpha_low_u*/
         0x03FF03FF,
         /*iircnr_alpha_low_v*/
         0x0000005C,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000004,

        },
        /*bypass*/
        0x080F0201,
    },
    /*param6.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0004,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x04C80660 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x04C80660, 0x00000066, 0x5F3F1300, 0xDFBF9F7F, 0x10101010,
            0x10101010, 0x07033A32, 0x00000703 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x00FF, 0x00FF, 0x00FF, 0x03FF, 0x03FF, 0x03FF, 0x005C,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0074,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x005C,
         /*uv_low_thr1*/
         {
             0x0000, 0x4040, 0x4040, 0x4040, 0x4040, 0x1002, 0xFFFF,
             0xFFFF /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x05,
         /*iircnr_y_th*/
         0x05,
         /*iircnr_y_min_th*/
         0x05,
         /*iircnr_uv_diff_thr*/
         0x05,
         /*y_edge_thr_min*/
         {
             0x0505, 0x0505, 0x1905, 0x5F3F, 0x9F7F, 0xDFBF, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x080F0100,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param7.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0660, 0x04C8, 0x0660, 0x04C8, 0x0066,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x1300, 0x5F3F, 0x9F7F, 0xDFBF, 0x1010, 0x1010, 0x1010,
            0x1010 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x07033A32, 0x00000703, 0x00000000, 0x00FF00FF, 0x03FF00FF,
            0x03FF03FF, 0x0000005C, 0x00000074 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x0000005C, 0x40404040, 0x40404040, 0xFFFF1002, 0xFFFFFFFF,
            0x05050505, 0x05050505, 0x5F3F1905 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x9F7F,
            /*iircnr_sat_ratio*/
            0xDFBF,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100,
             0x080F /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0004,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0100,
         /*uv_low_thr1*/
         {
             0x080F, 0x0004, 0x0000, 0x0201, 0x080F, 0x0004, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x10101010,

        },
        /*bypass*/
        0x07033A32,
    },
    /*param8.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0703,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00FF00FF,
        /*ymd_min_u*/
        0x03FF00FF,
        /*ymd_min_v*/
        0x03FF03FF,
        /*slop_uv*/
        {
            0x005C, 0x0000, 0x0074, 0x0000, 0x005C, 0x0000, 0x4040,
            0x4040 /*0-7*/
        },
        /*slop_y*/
        {
            0x4040, 0x4040, 0x1002, 0xFFFF, 0xFFFF, 0xFFFF, 0x0505,
            0x0505 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x05050505, 0x5F3F1905, 0xDFBF9F7F, 0x00000000, 0x00000000,
            0x00000000, 0x080F0100, 0x00000004 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x080F0100, 0x00000004, 0x080F0201, 0x00000004, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0660,
             0x04C8 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0660,
         /*iircnr_alpha_hl_diff_v*/
         0x04C8,
         /*iircnr_alpha_low_u*/
         0x00000066,
         /*iircnr_alpha_low_v*/
         0x5F3F1300,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x05050505,

        },
        /*bypass*/
        0x05050505,
    },
    /*param9.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x1905,
        /*cnr_uv_pg_th*/
        0x5F3F,
        /*ymd_u*/
        0xDFBF9F7F,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0100, 0x080F, 0x0004, 0x0000, 0x0100, 0x080F, 0x0004,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0201, 0x080F, 0x0004, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0660, 0x04C8, 0x0660, 0x04C8, 0x0066, 0x0000, 0x1300,
             0x5F3F /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x9F7F,
         /*iircnr_uv_th*/
         0xDFBF,
         /*iircnr_uv_dist*/
         0x1010,
         /*uv_low_thr1*/
         {
             0x1010, 0x1010, 0x1010, 0x3A32, 0x0703, 0x0703, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0xFF,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0xFF,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x00FF, 0x03FF, 0x03FF, 0x03FF, 0x005C, 0x0000, 0x0074,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x005C,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x40404040,
         /*iircnr_alpha_low_v*/
         0x40404040,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param10.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x04C80660, 0x04C80660,
            0x00000066, 0x5F3F1300, 0xDFBF9F7F /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x10101010, 0x10101010, 0x07033A32, 0x00000703, 0x00000000,
            0x00FF00FF, 0x03FF00FF, 0x03FF03FF /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x005C,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0074, 0x0000, 0x005C, 0x0000, 0x4040, 0x4040, 0x4040,
             0x4040 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x1002,
         /*iircnr_uv_th*/
         0xFFFF,
         /*iircnr_uv_dist*/
         0xFFFF,
         /*uv_low_thr1*/
         {
             0xFFFF, 0x0505, 0x0505, 0x0505, 0x0505, 0x1905, 0x5F3F,
             0x9F7F /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x080F, 0x0004,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0100,
         /*iircnr_alpha_hl_diff_v*/
         0x080F,
         /*iircnr_alpha_low_u*/
         0x00000004,
         /*iircnr_alpha_low_v*/
         0x080F0201,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param11.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x04C80660,
        /*ymd_min_u*/
        0x04C80660,
        /*ymd_min_v*/
        0x00000066,
        /*slop_uv*/
        {
            0x1300, 0x5F3F, 0x9F7F, 0xDFBF, 0x1010, 0x1010, 0x1010,
            0x1010 /*0-7*/
        },
        /*slop_y*/
        {
            0x3A32, 0x0703, 0x0703, 0x0000, 0x0000, 0x0000, 0x00FF,
            0x00FF /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x03FF00FF, 0x03FF03FF, 0x0000005C, 0x00000074, 0x0000005C,
            0x40404040, 0x40404040, 0xFFFF1002 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0xFFFFFFFF, 0x05050505, 0x05050505, 0x5F3F1905, 0xDFBF9F7F,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0100,
            /*iircnr_sat_ratio*/
            0x080F,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0004, 0x0000, 0x0100, 0x080F, 0x0004, 0x0000, 0x0201,
             0x080F /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0004,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00FF00FF,

        },
        /*bypass*/
        0x03FF00FF,
    },
    /*param12.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x03FF,
        /*cnr_uv_pg_th*/
        0x03FF,
        /*ymd_u*/
        0x0000005C,
        /*ymd_v*/
        0x00000074,
        /*ymd_min_u*/
        0x0000005C,
        /*ymd_min_v*/
        0x40404040,
        /*slop_uv*/
        {
            0x4040, 0x4040, 0x1002, 0xFFFF, 0xFFFF, 0xFFFF, 0x0505,
            0x0505 /*0-7*/
        },
        /*slop_y*/
        {
            0x0505, 0x0505, 0x1905, 0x5F3F, 0x9F7F, 0xDFBF, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x080F0100, 0x00000004, 0x080F0100,
            0x00000004, 0x080F0201, 0x00000004 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x60,
         /*iircnr_y_th*/
         0x06,
         /*iircnr_y_min_th*/
         0xC8,
         /*iircnr_uv_diff_thr*/
         0x04,
         /*y_edge_thr_min*/
         {
             0x0660, 0x04C8, 0x0066, 0x0000, 0x1300, 0x5F3F, 0x9F7F,
             0xDFBF /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x1010,
         /*iircnr_alpha_hl_diff_v*/
         0x1010,
         /*iircnr_alpha_low_u*/
         0x10101010,
         /*iircnr_alpha_low_v*/
         0x07033A32,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param13.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x080F0100,
        /*ymd_v*/
        0x00000004,
        /*ymd_min_u*/
        0x080F0100,
        /*ymd_min_v*/
        0x00000004,
        /*slop_uv*/
        {
            0x0201, 0x080F, 0x0004, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x04C80660, 0x04C80660, 0x00000066 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x1300,
            /*iircnr_sat_ratio*/
            0x5F3F,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x9F7F, 0xDFBF, 0x1010, 0x1010, 0x1010, 0x1010, 0x3A32,
             0x0703 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0703,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x00FF, 0x00FF, 0x00FF, 0x03FF, 0x03FF, 0x03FF,
             0x005C /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x74,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x005C, 0x0000, 0x4040, 0x4040, 0x4040, 0x4040, 0x1002,
             0xFFFF /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0xFFFF,
         /*iircnr_alpha_hl_diff_v*/
         0xFFFF,
         /*iircnr_alpha_low_u*/
         0x05050505,
         /*iircnr_alpha_low_v*/
         0x05050505,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param14.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0660,
            0x04C8 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x04C80660, 0x00000066, 0x5F3F1300, 0xDFBF9F7F, 0x10101010,
            0x10101010, 0x07033A32, 0x00000703 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00FF00FF, 0x03FF00FF, 0x03FF03FF, 0x0000005C,
            0x00000074, 0x0000005C, 0x40404040 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x4040,
            /*iircnr_sat_ratio*/
            0x4040,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x1002, 0xFFFF, 0xFFFF, 0xFFFF, 0x0505, 0x0505, 0x0505,
             0x0505 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x1905,
         /*iircnr_uv_th*/
         0x5F3F,
         /*iircnr_uv_dist*/
         0x9F7F,
         /*uv_low_thr1*/
         {
             0xDFBF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0100 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x04,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0100, 0x080F, 0x0004, 0x0000, 0x0201, 0x080F, 0x0004,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x04C80660,

        },
        /*bypass*/
        0x04C80660,
    },
    /*param15.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0066,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x5F3F1300,
        /*ymd_v*/
        0xDFBF9F7F,
        /*ymd_min_u*/
        0x10101010,
        /*ymd_min_v*/
        0x10101010,
        /*slop_uv*/
        {
            0x3A32, 0x0703, 0x0703, 0x0000, 0x0000, 0x0000, 0x00FF,
            0x00FF /*0-7*/
        },
        /*slop_y*/
        {
            0x00FF, 0x03FF, 0x03FF, 0x03FF, 0x005C, 0x0000, 0x0074,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x0000005C, 0x40404040, 0x40404040, 0xFFFF1002, 0xFFFFFFFF,
            0x05050505, 0x05050505, 0x5F3F1905 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0xDFBF9F7F, 0x00000000, 0x00000000, 0x00000000, 0x080F0100,
            0x00000004, 0x080F0100, 0x00000004 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0201,
            /*iircnr_sat_ratio*/
            0x080F,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0004, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000074,

        },
        /*bypass*/
        0x0000005C,
    },
    /*param16.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x4040,
        /*cnr_uv_pg_th*/
        0x4040,
        /*ymd_u*/
        0x40404040,
        /*ymd_v*/
        0xFFFF1002,
        /*ymd_min_u*/
        0xFFFFFFFF,
        /*ymd_min_v*/
        0x05050505,
        /*slop_uv*/
        {
            0x0505, 0x0505, 0x1905, 0x5F3F, 0x9F7F, 0xDFBF, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x080F, 0x0004,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x080F0100, 0x00000004, 0x080F0201, 0x00000004, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0660, 0x04C8, 0x0660, 0x04C8, 0x0066, 0x0000,
             0x1300 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x7F,
         /*iircnr_y_th*/
         0x9F,
         /*iircnr_y_min_th*/
         0xBF,
         /*iircnr_uv_diff_thr*/
         0xDF,
         /*y_edge_thr_min*/
         {
             0x1010, 0x1010, 0x1010, 0x1010, 0x3A32, 0x0703, 0x0703,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00FF00FF,
         /*iircnr_alpha_low_v*/
         0x03FF00FF,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000004,

        },
        /*bypass*/
        0x080F0100,
    },
    /*param17.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0004,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x080F0201,
        /*ymd_v*/
        0x00000004,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x04C80660, 0x04C80660, 0x00000066, 0x5F3F1300,
            0xDFBF9F7F, 0x10101010, 0x10101010 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x3A32,
            /*iircnr_sat_ratio*/
            0x0703,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0703, 0x0000, 0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF,
             0x03FF /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x03FF,
         /*iircnr_uv_th*/
         0x03FF,
         /*iircnr_uv_dist*/
         0x005C,
         /*uv_low_thr1*/
         {
             0x0000, 0x0074, 0x0000, 0x005C, 0x0000, 0x4040, 0x4040,
             0x4040 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x02,
         /*iircnr_y_th*/
         0x10,
         /*iircnr_y_min_th*/
         0xFF,
         /*iircnr_uv_diff_thr*/
         0xFF,
         /*y_edge_thr_min*/
         {
             0xFFFF, 0xFFFF, 0x0505, 0x0505, 0x0505, 0x0505, 0x1905,
             0x5F3F /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x9F7F,
         /*iircnr_alpha_hl_diff_v*/
         0xDFBF,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param18.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0660,
            0x04C8 /*0-7*/
        },
        /*slop_y*/
        {
            0x0660, 0x04C8, 0x0066, 0x0000, 0x1300, 0x5F3F, 0x9F7F,
            0xDFBF /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x10101010, 0x10101010, 0x07033A32, 0x00000703, 0x00000000,
            0x00FF00FF, 0x03FF00FF, 0x03FF03FF /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x0000005C, 0x00000074, 0x0000005C, 0x40404040, 0x40404040,
            0xFFFF1002, 0xFFFFFFFF, 0x05050505 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0505,
            /*iircnr_sat_ratio*/
            0x0505,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x1905, 0x5F3F, 0x9F7F, 0xDFBF, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0100,
         /*uv_low_thr1*/
         {
             0x080F, 0x0004, 0x0000, 0x0100, 0x080F, 0x0004, 0x0000,
             0x0201 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x04,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0xDFBF9F7F,

        },
        /*bypass*/
        0x10101010,
    },
    /*param19.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x1010,
        /*cnr_uv_pg_th*/
        0x1010,
        /*ymd_u*/
        0x07033A32,
        /*ymd_v*/
        0x00000703,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00FF00FF,
        /*slop_uv*/
        {
            0x00FF, 0x03FF, 0x03FF, 0x03FF, 0x005C, 0x0000, 0x0074,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x005C, 0x0000, 0x4040, 0x4040, 0x4040, 0x4040, 0x1002,
            0xFFFF /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0xFFFFFFFF, 0x05050505, 0x05050505, 0x5F3F1905, 0xDFBF9F7F,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x080F0100, 0x00000004, 0x080F0100, 0x00000004, 0x080F0201,
            0x00000004, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x04C80660,
         /*iircnr_alpha_low_v*/
         0x04C80660,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0xFFFF1002,

        },
        /*bypass*/
        0xFFFFFFFF,
    },
    /*param20.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0505,
        /*cnr_uv_pg_th*/
        0x0505,
        /*ymd_u*/
        0x05050505,
        /*ymd_v*/
        0x5F3F1905,
        /*ymd_min_u*/
        0xDFBF9F7F,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x080F, 0x0004,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0100, 0x080F, 0x0004, 0x0000, 0x0201, 0x080F, 0x0004,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0660, 0x04C8, 0x0660,
             0x04C8 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0066,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x1300,
         /*uv_low_thr1*/
         {
             0x5F3F, 0x9F7F, 0xDFBF, 0x1010, 0x1010, 0x1010, 0x1010,
             0x3A32 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x03,
         /*iircnr_y_th*/
         0x07,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x03FF, 0x03FF,
             0x03FF /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x005C,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000074,
         /*iircnr_alpha_low_v*/
         0x0000005C,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000004,

        },
        /*bypass*/
        0x00000000,
    },
    /*param21.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x00000000,
        /*slop_uv*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x04C80660, 0x04C80660, 0x00000066 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x5F3F1300, 0xDFBF9F7F, 0x10101010, 0x10101010, 0x07033A32,
            0x00000703, 0x00000000, 0x00FF00FF /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x00FF,
            /*iircnr_sat_ratio*/
            0x03FF,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x03FF, 0x03FF, 0x005C, 0x0000, 0x0074, 0x0000, 0x005C,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x4040,
         /*iircnr_uv_th*/
         0x4040,
         /*iircnr_uv_dist*/
         0x4040,
         /*uv_low_thr1*/
         {
             0x4040, 0x1002, 0xFFFF, 0xFFFF, 0xFFFF, 0x0505, 0x0505,
             0x0505 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x05,
         /*iircnr_y_th*/
         0x19,
         /*iircnr_y_min_th*/
         0x3F,
         /*iircnr_uv_diff_thr*/
         0x5F,
         /*y_edge_thr_min*/
         {
             0x9F7F, 0xDFBF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0100,
         /*iircnr_alpha_hl_diff_v*/
         0x080F,
         /*iircnr_alpha_low_u*/
         0x00000004,
         /*iircnr_alpha_low_v*/
         0x080F0100,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
    /*param22.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x00000000,
        /*ymd_min_v*/
        0x04C80660,
        /*slop_uv*/
        {
            0x0660, 0x04C8, 0x0066, 0x0000, 0x1300, 0x5F3F, 0x9F7F,
            0xDFBF /*0-7*/
        },
        /*slop_y*/
        {
            0x1010, 0x1010, 0x1010, 0x1010, 0x3A32, 0x0703, 0x0703,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00FF00FF, 0x03FF00FF, 0x03FF03FF, 0x0000005C,
            0x00000074, 0x0000005C, 0x40404040 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x40404040, 0xFFFF1002, 0xFFFFFFFF, 0x05050505, 0x05050505,
            0x5F3F1905, 0xDFBF9F7F, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0100, 0x080F, 0x0004, 0x0000, 0x0100,
             0x080F /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0004,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0201,
         /*uv_low_thr1*/
         {
             0x080F, 0x0004, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x0000,
         /*iircnr_alpha_hl_diff_v*/
         0x0000,
         /*iircnr_alpha_low_u*/
         0x00000000,
         /*iircnr_alpha_low_v*/
         0x00000000,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x04C80660,
              /*uv_high_thr2*/
              0x04C80660,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000066,
              /*uv_high_thr2*/
              0x5F3F1300,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0xDFBF9F7F,
              /*uv_high_thr2*/
              0x10101010,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000703,

        },
        /*bypass*/
        0x00000000,
    },
    /*param23.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x00FF,
        /*cnr_uv_pg_th*/
        0x00FF,
        /*ymd_u*/
        0x03FF00FF,
        /*ymd_v*/
        0x03FF03FF,
        /*ymd_min_u*/
        0x0000005C,
        /*ymd_min_v*/
        0x00000074,
        /*slop_uv*/
        {
            0x005C, 0x0000, 0x4040, 0x4040, 0x4040, 0x4040, 0x1002,
            0xFFFF /*0-7*/
        },
        /*slop_y*/
        {
            0xFFFF, 0xFFFF, 0x0505, 0x0505, 0x0505, 0x0505, 0x1905,
            0x5F3F /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0xDFBF9F7F, 0x00000000, 0x00000000, 0x00000000, 0x080F0100,
            0x00000004, 0x080F0100, 0x00000004 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x080F0201, 0x00000004, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0000,
            /*iircnr_sat_ratio*/
            0x0000,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x0000,
         /*iircnr_uv_th*/
         0x0000,
         /*iircnr_uv_dist*/
         0x0000,
         /*uv_low_thr1*/
         {
             0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
             0x0000 /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0x00,
         /*iircnr_y_th*/
         0x00,
         /*iircnr_y_min_th*/
         0x00,
         /*iircnr_uv_diff_thr*/
         0x00,
         /*y_edge_thr_min*/
         {
             0x0000, 0x0000, 0x0660, 0x04C8, 0x0660, 0x04C8, 0x0066,
             0x0000 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x1300,
         /*iircnr_alpha_hl_diff_v*/
         0x5F3F,
         /*iircnr_alpha_low_u*/
         0xDFBF9F7F,
         /*iircnr_alpha_low_v*/
         0x10101010,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x10101010,
              /*uv_high_thr2*/
              0x07033A32,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x00000703,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00FF00FF,
              /*uv_high_thr2*/
              0x03FF00FF,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x03FF03FF,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000074,
              /*uv_high_thr2*/
              0x0000005C,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x40404040,
              /*uv_high_thr2*/
              0x40404040,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0xFFFF1002,
              /*uv_high_thr2*/
              0xFFFFFFFF,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x5F3F1905,

        },
        /*bypass*/
        0xDFBF9F7F,
    },
    /*param24.*/
    /*v21_sensor_iircnr_level*/
    {
        /*cnr_iir_mode*/
        0x0000,
        /*cnr_uv_pg_th*/
        0x0000,
        /*ymd_u*/
        0x00000000,
        /*ymd_v*/
        0x00000000,
        /*ymd_min_u*/
        0x080F0100,
        /*ymd_min_v*/
        0x00000004,
        /*slop_uv*/
        {
            0x0100, 0x080F, 0x0004, 0x0000, 0x0201, 0x080F, 0x0004,
            0x0000 /*0-7*/
        },
        /*slop_y*/
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000 /*0-7*/
        },
        /*middle_factor_uv*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000 /*0-7*/
        },
        /*middle_factor_y*/
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x04C80660 /*0-7*/
        },
        /*pre_uv_th*/
        {
            /*iircnr_pre_uv_th*/
            0x0660,
            /*iircnr_sat_ratio*/
            0x04C8,

        },
        /*iircnr_ee*/
        {/*y_edge_thr_max*/
         {
             0x0066, 0x0000, 0x1300, 0x5F3F, 0x9F7F, 0xDFBF, 0x1010,
             0x1010 /*0-7*/
         },
         /*iircnr_uv_s_th*/
         0x1010,
         /*iircnr_uv_th*/
         0x1010,
         /*iircnr_uv_dist*/
         0x3A32,
         /*uv_low_thr1*/
         {
             0x0703, 0x0703, 0x0000, 0x0000, 0x0000, 0x00FF, 0x00FF,
             0x00FF /*0-7*/
         }},
        /*iircnr_str*/
        {/*iircnr_y_max_th*/
         0xFF,
         /*iircnr_y_th*/
         0x03,
         /*iircnr_y_min_th*/
         0xFF,
         /*iircnr_uv_diff_thr*/
         0x03,
         /*y_edge_thr_min*/
         {
             0x005C, 0x0000, 0x0074, 0x0000, 0x005C, 0x0000, 0x4040,
             0x4040 /*0-7*/
         },
         /*iircnr_alpha_hl_diff_u*/
         0x4040,
         /*iircnr_alpha_hl_diff_v*/
         0x4040,
         /*iircnr_alpha_low_u*/
         0xFFFF1002,
         /*iircnr_alpha_low_v*/
         0xFFFFFFFF,
         /*cnr_uv_thr2*/
         {/*[0x0]*/
          {
              /*uv_low_thr2*/
              0x05050505,
              /*uv_high_thr2*/
              0x05050505,

          },
          /*[0x1]*/
          {
              /*uv_low_thr2*/
              0x5F3F1905,
              /*uv_high_thr2*/
              0xDFBF9F7F,

          },
          /*[0x2]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x3]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x4]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0100,

          },
          /*[0x5]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x080F0201,

          },
          /*[0x6]*/
          {
              /*uv_low_thr2*/
              0x00000004,
              /*uv_high_thr2*/
              0x00000000,

          },
          /*[0x7]*/
          {
              /*uv_low_thr2*/
              0x00000000,
              /*uv_high_thr2*/
              0x00000000,

          }}},
        /*css_lum_thr*/
        {
            /*iircnr_css_lum_thr*/
            0x00000000,

        },
        /*bypass*/
        0x00000000,
    },
