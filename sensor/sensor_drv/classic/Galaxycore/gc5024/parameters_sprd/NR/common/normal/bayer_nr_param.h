/*versionid=0x00070005*/
/*maxGain=0.00*/
/*param0.&BasePoint=1&*/
/*v21_sensor_nlm_level*/
{
    /*first_lum*/
    {
        /*nlm_flat_opt_bypass*/
        0x00,
            /*flat_opt_mode*/
            0x00,
            /*first_lum_bypass*/
            0x00,
            /*reserved*/
            0x00,
            /*lum_thr0*/
            0x00C8,
            /*lum_thr1*/
            0x01F4,
        /*nlm_lum*/
        {
            /*[0x0]*/
            {
                /*nlm_flat*/
                {
                    /*[0x0]*/
                    {
                        /*flat_inc_str*/
                        0x19,
                            /*flat_match_cnt*/
                            0x15,
                            /*flat_thresh*/
                            0x00B4,
                            /*addback0*/
                            0x0064,
                            /*addback1*/
                            0x0064,
                            /*addback_clip_max*/
                            0x0008,
                            /*addback_clip_min*/
                            0xFFF8,
                    }
                    ,
                        /*[0x1]*/
                        {
                            /*flat_inc_str*/
                            0x0F,
                            /*flat_match_cnt*/
                            0x15,
                            /*flat_thresh*/
                            0x00DC,
                            /*addback0*/
                            0x006E,
                            /*addback1*/
                            0x006E,
                            /*addback_clip_max*/
                            0x000A,
                            /*addback_clip_min*/
                            0xFFF6,

                        },
                    /*[0x2]*/
                    {
                        /*flat_inc_str*/
                        0x00,
                            /*flat_match_cnt*/
                            0x10,
                            /*flat_thresh*/
                            0x0104,
                            /*addback0*/
                            0x006E,
                            /*addback1*/
                            0x006E,
                            /*addback_clip_max*/
                            0x000A,
                            /*addback_clip_min*/
                            0xFFF6,
                    }
                }
                ,
                /*nlm_texture*/
                {
                    /*texture_dec_str*/
                    0x3F,
                        /*addback30*/
                        0x40,
                        /*addback31*/
                        0x40,
                        /*reserved*/
                        0x00,
                        /*addback_clip_max*/
                        0x000A,
                        /*addback_clip_min*/
                        0xFFF6,
                }
            }
            ,
                /*[0x1]*/
                { /*nlm_flat*/
                 {/*[0x0]*/
                  {
                      /*flat_inc_str*/
                      0x14,
                      /*flat_match_cnt*/
                      0x15,
                      /*flat_thresh*/
                      0x00A0,
                      /*addback0*/
                      0x0064,
                      /*addback1*/
                      0x0064,
                      /*addback_clip_max*/
                      0x0008,
                      /*addback_clip_min*/
                      0xFFF8,

                  },
                  /*[0x1]*/
                  {
                      /*flat_inc_str*/
                      0x0C,
                      /*flat_match_cnt*/
                      0x15,
                      /*flat_thresh*/
                      0x00C8,
                      /*addback0*/
                      0x006E,
                      /*addback1*/
                      0x006E,
                      /*addback_clip_max*/
                      0x000A,
                      /*addback_clip_min*/
                      0xFFF6,

                  },
                  /*[0x2]*/
                  {
                      /*flat_inc_str*/
                      0xFB,
                      /*flat_match_cnt*/
                      0x10,
                      /*flat_thresh*/
                      0x00F0,
                      /*addback0*/
                      0x006E,
                      /*addback1*/
                      0x006E,
                      /*addback_clip_max*/
                      0x000A,
                      /*addback_clip_min*/
                      0xFFF6,

                  }},
                 /*nlm_texture*/
                 {
                     /*texture_dec_str*/
                     0x3F,
                     /*addback30*/
                     0x40,
                     /*addback31*/
                     0x40,
                     /*reserved*/
                     0x00,
                     /*addback_clip_max*/
                     0x000A,
                     /*addback_clip_min*/
                     0xFFF6,

                 }},
            /*[0x2]*/
            {
                /*nlm_flat*/
                {
                    /*[0x0]*/
                    {
                        /*flat_inc_str*/
                        0x0A,
                            /*flat_match_cnt*/
                            0x15,
                            /*flat_thresh*/
                            0x00B4,
                            /*addback0*/
                            0x0064,
                            /*addback1*/
                            0x0064,
                            /*addback_clip_max*/
                            0x000A,
                            /*addback_clip_min*/
                            0xFFF6,
                    }
                    ,
                        /*[0x1]*/
                        {
                            /*flat_inc_str*/
                            0xF6,
                            /*flat_match_cnt*/
                            0x15,
                            /*flat_thresh*/
                            0x00DC,
                            /*addback0*/
                            0x006E,
                            /*addback1*/
                            0x006E,
                            /*addback_clip_max*/
                            0x000C,
                            /*addback_clip_min*/
                            0xFFF4,

                        },
                    /*[0x2]*/
                    {
                        /*flat_inc_str*/
                        0xE0,
                            /*flat_match_cnt*/
                            0x10,
                            /*flat_thresh*/
                            0x0104,
                            /*addback0*/
                            0x006E,
                            /*addback1*/
                            0x006E,
                            /*addback_clip_max*/
                            0x000C,
                            /*addback_clip_min*/
                            0xFFF4,
                    }
                }
                ,
                /*nlm_texture*/
                {
                    /*texture_dec_str*/
                    0x3F,
                        /*addback30*/
                        0x40,
                        /*addback31*/
                        0x40,
                        /*reserved*/
                        0x00,
                        /*addback_clip_max*/
                        0x000C,
                        /*addback_clip_min*/
                        0xFFF4,
                }
            }
        }
    }
    ,
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003E1, 0x0000038B, 0x0000030D, 0x00000279,
             0x000001E3, 0x0000015B, 0x000000EB, 0x00000096, 0x0000005A,
             0x00000033, 0x0000001B, 0x0000000E, 0x00000006, 0x00000003,
             0x00000001, /*0-15*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
}
,
    /*param1.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00C8,
                /*addback0*/
                0x0064,
                /*addback1*/
                0x0064,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00F0,
                /*addback0*/
                0x0064,
                /*addback1*/
                0x0064,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0118,
                /*addback0*/
                0x006E,
                /*addback1*/
                0x006E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x14,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00B4,
                /*addback0*/
                0x0064,
                /*addback1*/
                0x0064,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00DC,
                /*addback0*/
                0x0064,
                /*addback1*/
                0x0064,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0104,
                /*addback0*/
                0x006E,
                /*addback1*/
                0x006E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x0A,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00C8,
                /*addback0*/
                0x0064,
                /*addback1*/
                0x0064,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00F0,
                /*addback0*/
                0x0064,
                /*addback1*/
                0x0064,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0118,
                /*addback0*/
                0x006E,
                /*addback1*/
                0x006E,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003E9, 0x000003AA, 0x0000034A, 0x000002D3,
             0x00000253, 0x000001D5, 0x00000162, 0x00000100, 0x000000B1,
             0x00000075, 0x0000004A, 0x0000002D, 0x0000001A, 0x0000000F,
             0x00000008, /*0-15*/
             0x00000004, 0x00000002, 0x00000001, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param2.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x010E,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0136,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x1E,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0069,
                /*addback1*/
                0x0069,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0122,
                /*addback0*/
                0x0069,
                /*addback1*/
                0x0069,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0069,
                /*addback1*/
                0x0069,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x14,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x010E,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0136,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F1, 0x000003C7, 0x00000385, 0x00000330,
             0x000002CF, 0x00000268, 0x00000201, 0x0000019F, 0x00000147,
             0x000000FA, 0x000000BA, 0x00000086, 0x0000005E, 0x00000041,
             0x0000002B, /*0-15*/
             0x0000001C, 0x00000011, 0x0000000B, 0x00000006, 0x00000004,
             0x00000002, 0x00000001, 0x00000001, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param3.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0118,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F4, 0x000003D5, 0x000003A3, 0x00000361,
             0x00000313, 0x000002BD, 0x00000263, 0x0000020A, 0x000001B5,
             0x00000166, 0x0000011F, 0x000000E1, 0x000000AD, 0x00000082,
             0x00000060, /*0-15*/
             0x00000045, 0x00000031, 0x00000022, 0x00000017, 0x0000000F,
             0x0000000A, 0x00000006, 0x00000004, 0x00000002, 0x00000001,
             0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param4.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DD, 0x000003B4, 0x0000037E,
             0x0000033D, 0x000002F4, 0x000002A6, 0x00000255, 0x00000206,
             0x000001B9, 0x00000172, 0x00000131, 0x000000F7, 0x000000C5,
             0x0000009A, /*0-15*/
             0x00000077, 0x0000005A, 0x00000043, 0x00000031, 0x00000023,
             0x00000019, 0x00000012, 0x0000000C, 0x00000008, 0x00000005,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000001,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param5.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F8, 0x000003E3, 0x000003C0, 0x00000392,
             0x0000035A, 0x0000031B, 0x000002D6, 0x0000028D, 0x00000244,
             0x000001FB, 0x000001B6, 0x00000175, 0x00000139, 0x00000103,
             0x000000D3, /*0-15*/
             0x000000AA, 0x00000087, 0x00000069, 0x00000051, 0x0000003E,
             0x0000002E, 0x00000022, 0x00000019, 0x00000012, 0x0000000D,
             0x00000009, 0x00000006, 0x00000004, 0x00000003, 0x00000002,
             0x00000001, /*16-31*/
             0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param6.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F9, 0x000003E7, 0x000003C9, 0x000003A1,
             0x00000370, 0x00000338, 0x000002FA, 0x000002B8, 0x00000274,
             0x00000230, 0x000001EE, 0x000001AE, 0x00000172, 0x0000013A,
             0x00000108, /*0-15*/
             0x000000DB, 0x000000B3, 0x00000091, 0x00000074, 0x0000005C,
             0x00000048, 0x00000037, 0x0000002A, 0x00000020, 0x00000018,
             0x00000011, 0x0000000D, 0x00000009, 0x00000006, 0x00000005,
             0x00000003, /*16-31*/
             0x00000002, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param7.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FA, 0x000003EA, 0x000003D0, 0x000003AC,
             0x00000380, 0x0000034E, 0x00000316, 0x000002DA, 0x0000029B,
             0x0000025B, 0x0000021C, 0x000001DE, 0x000001A3, 0x0000016B,
             0x00000138, /*0-15*/
             0x00000109, 0x000000DE, 0x000000B9, 0x00000098, 0x0000007C,
             0x00000064, 0x0000004F, 0x0000003F, 0x00000031, 0x00000026,
             0x0000001D, 0x00000016, 0x00000010, 0x0000000C, 0x00000009,
             0x00000006, /*16-31*/
             0x00000005, 0x00000003, 0x00000002, 0x00000002, 0x00000001,
             0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param8.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FA, 0x000003EC, 0x000003D5, 0x000003B5,
             0x0000038E, 0x00000360, 0x0000032C, 0x000002F5, 0x000002BB,
             0x0000027F, 0x00000243, 0x00000208, 0x000001CE, 0x00000197,
             0x00000163, /*0-15*/
             0x00000133, 0x00000107, 0x000000DF, 0x000000BB, 0x0000009C,
             0x00000081, 0x00000069, 0x00000055, 0x00000044, 0x00000036,
             0x0000002B, 0x00000021, 0x0000001A, 0x00000014, 0x0000000F,
             0x0000000B, /*16-31*/
             0x00000008, 0x00000006, 0x00000004, 0x00000003, 0x00000002,
             0x00000002, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param9.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FB, 0x000003EE, 0x000003D9, 0x000003BC,
             0x00000398, 0x0000036E, 0x0000033F, 0x0000030C, 0x000002D6,
             0x0000029D, 0x00000264, 0x0000022B, 0x000001F4, 0x000001BD,
             0x0000018A, /*0-15*/
             0x00000159, 0x0000012C, 0x00000103, 0x000000DD, 0x000000BB,
             0x0000009E, 0x00000083, 0x0000006C, 0x00000059, 0x00000048,
             0x0000003A, 0x0000002E, 0x00000025, 0x0000001D, 0x00000016,
             0x00000011, /*16-31*/
             0x0000000D, 0x0000000A, 0x00000008, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000002, 0x00000001, 0x00000001,
             0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param10.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FB, 0x000003EF, 0x000003DC, 0x000003C2,
             0x000003A1, 0x0000037A, 0x0000034F, 0x0000031F, 0x000002EC,
             0x000002B7, 0x00000281, 0x0000024A, 0x00000215, 0x000001E0,
             0x000001AD, /*0-15*/
             0x0000017C, 0x0000014F, 0x00000125, 0x000000FE, 0x000000DA,
             0x000000BA, 0x0000009E, 0x00000085, 0x0000006F, 0x0000005B,
             0x0000004B, 0x0000003D, 0x00000031, 0x00000028, 0x00000020,
             0x00000019, /*16-31*/
             0x00000014, 0x0000000F, 0x0000000C, 0x00000009, 0x00000007,
             0x00000005, 0x00000004, 0x00000003, 0x00000002, 0x00000002,
             0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param11.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FB, 0x000003F1, 0x000003DF, 0x000003C7,
             0x000003A8, 0x00000384, 0x0000035C, 0x0000032F, 0x00000300,
             0x000002CD, 0x0000029A, 0x00000266, 0x00000232, 0x000001FE,
             0x000001CC, /*0-15*/
             0x0000019D, 0x0000016F, 0x00000144, 0x0000011C, 0x000000F8,
             0x000000D6, 0x000000B8, 0x0000009D, 0x00000085, 0x0000006F,
             0x0000005D, 0x0000004D, 0x0000003F, 0x00000034, 0x0000002A,
             0x00000022, /*16-31*/
             0x0000001B, 0x00000015, 0x00000011, 0x0000000D, 0x0000000A,
             0x00000008, 0x00000006, 0x00000005, 0x00000004, 0x00000003,
             0x00000002, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param12.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FC, 0x000003F2, 0x000003E1, 0x000003CB,
             0x000003AE, 0x0000038D, 0x00000367, 0x0000033D, 0x00000310,
             0x000002E1, 0x000002B0, 0x0000027E, 0x0000024C, 0x0000021A,
             0x000001E9, /*0-15*/
             0x000001BA, 0x0000018C, 0x00000161, 0x00000139, 0x00000113,
             0x000000F1, 0x000000D1, 0x000000B4, 0x0000009B, 0x00000084,
             0x0000006F, 0x0000005E, 0x0000004E, 0x00000041, 0x00000035,
             0x0000002C, /*16-31*/
             0x00000024, 0x0000001D, 0x00000017, 0x00000012, 0x0000000F,
             0x0000000B, 0x00000009, 0x00000007, 0x00000005, 0x00000004,
             0x00000003, 0x00000002, 0x00000002, 0x00000001, 0x00000001,
             0x00000001, /*32-47*/
             0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param13.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FC, 0x000003F3, 0x000003E3, 0x000003CE,
             0x000003B4, 0x00000395, 0x00000371, 0x0000034A, 0x0000031F,
             0x000002F2, 0x000002C3, 0x00000293, 0x00000263, 0x00000233,
             0x00000203, /*0-15*/
             0x000001D5, 0x000001A8, 0x0000017D, 0x00000154, 0x0000012E,
             0x0000010B, 0x000000EA, 0x000000CC, 0x000000B1, 0x00000098,
             0x00000082, 0x0000006F, 0x0000005E, 0x0000004F, 0x00000042,
             0x00000037, /*16-31*/
             0x0000002D, 0x00000025, 0x0000001E, 0x00000018, 0x00000014,
             0x00000010, 0x0000000D, 0x0000000A, 0x00000008, 0x00000006,
             0x00000005, 0x00000004, 0x00000003, 0x00000002, 0x00000002,
             0x00000001, /*32-47*/
             0x00000001, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param14.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x23,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0154,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xFB,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x0058,
                /*addback1*/
                0x0058,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0140,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0xF6,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0xE0,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x0062,
                /*addback1*/
                0x0062,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x40,
               /*addback31*/
               0x40,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000C,
               /*addback_clip_min*/
               0xFFF4,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x0C,
            /*simple_bpc_lum_thr*/
            0x00C8,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FC, 0x000003F3, 0x000003E5, 0x000003D1,
             0x000003B9, 0x0000039B, 0x0000037A, 0x00000354, 0x0000032C,
             0x00000301, 0x000002D5, 0x000002A7, 0x00000278, 0x00000249,
             0x0000021B, /*0-15*/
             0x000001ED, 0x000001C1, 0x00000196, 0x0000016E, 0x00000147,
             0x00000123, 0x00000101, 0x000000E2, 0x000000C6, 0x000000AC,
             0x00000095, 0x00000080, 0x0000006D, 0x0000005D, 0x0000004F,
             0x00000042, /*16-31*/
             0x00000037, 0x0000002E, 0x00000026, 0x0000001F, 0x00000019,
             0x00000015, 0x00000011, 0x0000000D, 0x0000000B, 0x00000008,
             0x00000007, 0x00000005, 0x00000004, 0x00000003, 0x00000002,
             0x00000002, /*32-47*/
             0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param15.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x2A,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x14,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x01AE,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x0A,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x01E0,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x30,
               /*addback31*/
               0x30,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0006,
               /*addback_clip_min*/
               0xFFFA,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x24,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0168,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x14,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x01CC,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x0A,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x01E0,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x0A,
               /*addback30*/
               0x30,
               /*addback31*/
               0x30,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0008,
               /*addback_clip_min*/
               0xFFF8,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x0A,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x014A,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x05,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000C,
                /*addback_clip_min*/
                0xFFF4,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x14,
               /*addback30*/
               0x3C,
               /*addback31*/
               0x3C,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x012C,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FC, 0x000003F3, 0x000003E4, 0x000003CF,
             0x000003B4, 0x00000395, 0x00000372, 0x0000034B, 0x00000321,
             0x000002F4, 0x000002C5, 0x00000296, 0x00000265, 0x00000235,
             0x00000206, /*0-15*/
             0x000001D7, 0x000001AB, 0x00000180, 0x00000157, 0x00000131,
             0x0000010D, 0x000000EC, 0x000000CE, 0x000000B3, 0x0000009A,
             0x00000084, 0x00000071, 0x0000005F, 0x00000050, 0x00000043,
             0x00000038, /*16-31*/
             0x0000002E, 0x00000026, 0x0000001F, 0x00000019, 0x00000014,
             0x00000010, 0x0000000D, 0x0000000A, 0x00000008, 0x00000006,
             0x00000005, 0x00000004, 0x00000003, 0x00000002, 0x00000002,
             0x00000001, /*32-47*/
             0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param16.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x00,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x00C8,
         /*lum_thr1*/
         0x01F4,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x01F4,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x19,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0258,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0xF6,
               /*addback30*/
               0x30,
               /*addback31*/
               0x30,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0006,
               /*addback_clip_min*/
               0xFFFA,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x28,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x017C,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x0008,
                /*addback_clip_min*/
                0xFFF8,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x1E,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x01E0,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x0C,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x01F4,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x0A,
               /*addback30*/
               0x30,
               /*addback31*/
               0x30,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0008,
               /*addback_clip_min*/
               0xFFF8,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x0A,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x05,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x10,
                /*flat_thresh*/
                0x0190,
                /*addback0*/
                0x003E,
                /*addback1*/
                0x003E,
                /*addback_clip_max*/
                0x000A,
                /*addback_clip_min*/
                0xFFF6,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x14,
               /*addback30*/
               0x3C,
               /*addback31*/
               0x3C,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x000A,
               /*addback_clip_min*/
               0xFFF6,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x02 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x0050,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x04,
            /*simple_bpc_lum_thr*/
            0x012C,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003FC, 0x000003F3, 0x000003E4, 0x000003CF,
             0x000003B4, 0x00000395, 0x00000372, 0x0000034B, 0x00000321,
             0x000002F4, 0x000002C5, 0x00000296, 0x00000265, 0x00000235,
             0x00000206, /*0-15*/
             0x000001D7, 0x000001AB, 0x00000180, 0x00000157, 0x00000131,
             0x0000010D, 0x000000EC, 0x000000CE, 0x000000B3, 0x0000009A,
             0x00000084, 0x00000071, 0x0000005F, 0x00000050, 0x00000043,
             0x00000038, /*16-31*/
             0x0000002E, 0x00000026, 0x0000001F, 0x00000019, 0x00000014,
             0x00000010, 0x0000000D, 0x0000000A, 0x00000008, 0x00000006,
             0x00000005, 0x00000004, 0x00000003, 0x00000002, 0x00000002,
             0x00000001, /*32-47*/
             0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x01,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param17.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param18.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param19.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param20.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param21.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param22.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param23.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
    /*param24.&BasePoint=1&*/
    /*v21_sensor_nlm_level*/
    {
        /*first_lum*/
        {/*nlm_flat_opt_bypass*/
         0x00,
         /*flat_opt_mode*/
         0x00,
         /*first_lum_bypass*/
         0x01,
         /*reserved*/
         0x00,
         /*lum_thr0*/
         0x0000,
         /*lum_thr1*/
         0x0000,
         /*nlm_lum*/
         {  /*[0x0]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x3F,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x00FA,
                /*addback0*/
                0x0020,
                /*addback1*/
                0x0020,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x30,
                /*flat_match_cnt*/
                0x15,
                /*flat_thresh*/
                0x015E,
                /*addback0*/
                0x0028,
                /*addback1*/
                0x0028,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x20,
                /*flat_match_cnt*/
                0x12,
                /*flat_thresh*/
                0x012C,
                /*addback0*/
                0x003F,
                /*addback1*/
                0x003F,
                /*addback_clip_max*/
                0x03FF,
                /*addback_clip_min*/
                0xFC00,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x3F,
               /*addback30*/
               0x3F,
               /*addback31*/
               0x3F,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x03FF,
               /*addback_clip_min*/
               0xFC00,

           }},
          /*[0x1]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }},
          /*[0x2]*/
          { /*nlm_flat*/
           {/*[0x0]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x1]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            },
            /*[0x2]*/
            {
                /*flat_inc_str*/
                0x00,
                /*flat_match_cnt*/
                0x00,
                /*flat_thresh*/
                0x0000,
                /*addback0*/
                0x0000,
                /*addback1*/
                0x0000,
                /*addback_clip_max*/
                0x0000,
                /*addback_clip_min*/
                0x0000,

            }},
           /*nlm_texture*/
           {
               /*texture_dec_str*/
               0x00,
               /*addback30*/
               0x00,
               /*addback31*/
               0x00,
               /*reserved*/
               0x00,
               /*addback_clip_max*/
               0x0000,
               /*addback_clip_min*/
               0x0000,

           }}}},
        /*nlm_dic*/
        {
            /*direction_mode_bypass*/
            0x00,
            /*dist_mode*/
            0x02,
            /*w_shift*/
            {
                0x02, 0x02, 0x03 /*0-2*/
            },
            /*cnt_th*/
            0x02,
            /*reserved*/
            {
                0x00, 0x00 /*0-1*/
            },
            /*diff_th*/
            0x003C,
            /*tdist_min_th*/
            0x008C,

        },
        /*simple_bpc*/
        {
            /*simple_bpc_bypass*/
            0x00,
            /*simple_bpc_thr*/
            0x08,
            /*simple_bpc_lum_thr*/
            0x0000,

        },
        /*lut_w*/
        {/*lut_w*/
         {
             0x000003FF, 0x000003F6, 0x000003DC, 0x000003B1, 0x00000378,
             0x00000335, 0x000002E9, 0x00000298, 0x00000246, 0x000001F5,
             0x000001A8, 0x00000160, 0x0000011F, 0x000000E6, 0x000000B6,
             0x0000008D, /*0-15*/
             0x0000006B, 0x00000050, 0x0000003B, 0x0000002A, 0x0000001E,
             0x00000015, 0x0000000E, 0x0000000A, 0x00000006, 0x00000004,
             0x00000003, 0x00000002, 0x00000001, 0x00000001, 0x00000000,
             0x00000000, /*16-31*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*32-47*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, /*48-63*/
             0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
             0x00000000, 0x00000000, 0x00000000 /*64-71*/
         }},
        /*nlm_den_strenth*/
        0x00,
        /*imp_opt_bypass*/
        0x00,
        /*vst_bypass*/
        0x00,
        /*nlm_bypass*/
        0x00,
    },
