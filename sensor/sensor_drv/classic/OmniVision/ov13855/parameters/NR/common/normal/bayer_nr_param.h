/*param0.*/
/*v21_sensor_nlm_level*/
{
    /*first_lum*/
    {
        /*nlm_flat_opt_bypass*/
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
        {
            /*[0x0]*/
            {
                /*nlm_flat*/
                {
                    /*[0x0]*/
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
                    }
                    ,
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
                    }
                }
                ,
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
                }
            }
            ,
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
            {
                /*nlm_flat*/
                {
                    /*[0x0]*/
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
                    }
                    ,
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
                    }
                }
                ,
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
            0x00,
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
}
,
    /*param1.*/
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
            0x00,
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
    /*param2.*/
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
            0x00,
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
    /*param3.*/
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
            0x00,
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
    /*param4.*/
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
            0x00,
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
    /*param5.*/
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
            0x00,
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
    /*param6.*/
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
            0x00,
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
    /*param7.*/
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
            0x00,
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
    /*param8.*/
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
            0x00,
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
    /*param9.*/
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
            0x00,
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
    /*param10.*/
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
            0x00,
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
    /*param11.*/
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
            0x00,
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
    /*param12.*/
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
            0x00,
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
    /*param13.*/
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
            0x00,
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
    /*param14.*/
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
            0x00,
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
    /*param15.*/
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
            0x00,
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
    /*param16.*/
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
            0x00,
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
    /*param17.*/
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
            0x00,
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
    /*param18.*/
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
            0x00,
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
    /*param19.*/
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
            0x00,
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
    /*param20.*/
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
            0x00,
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
    /*param21.*/
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
            0x00,
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
    /*param22.*/
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
            0x00,
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
    /*param23.*/
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
            0x00,
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
    /*param24.*/
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
            0x00,
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
