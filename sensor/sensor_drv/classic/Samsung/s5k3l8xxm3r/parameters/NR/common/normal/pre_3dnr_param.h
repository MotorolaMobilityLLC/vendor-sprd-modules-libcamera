/*versionid=0x00070005*/
/*maxGain=0.00*/
/*param0.&BasePoint=1&*/
/*v21_sensor_3dnr_level*/
{
    /*bypass*/
    0x00,
        /*fusion_mode*/
        0x00,
        /*filter_swt_en*/
        0x01,
        /*reserved*/
        0x00,
        /*yuv_cfg*/
        {/*grad_wgt_polyline*/
         {
             0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
             0x7F /*0-10*/
         },
         /*reserved*/
         0x00,
         /*y_cfg*/
         {/*src_wgt*/
          0x80,
          /*nr_thr*/
          0x05,
          /*nr_wgt*/
          0xFF,
          /*thr_polyline*/
          {
              0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
          },
          /*gain_polyline*/
          {
              0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
          },
          /*reserved*/
          {
              0x00, 0x00, 0x00 /*0-2*/
          }},
         /*u_cfg*/
         {/*src_wgt*/
          0x80,
          /*nr_thr*/
          0x03,
          /*nr_wgt*/
          0xFF,
          /*thr_polyline*/
          {
              0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
          },
          /*gain_polyline*/
          {
              0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
          },
          /*reserved*/
          {
              0x00, 0x00, 0x00 /*0-2*/
          }},
         /*v_cfg*/
         {/*src_wgt*/
          0x80,
          /*nr_thr*/
          0x03,
          /*nr_wgt*/
          0xFF,
          /*thr_polyline*/
          {
              0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
          },
          /*gain_polyline*/
          {
              0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
          },
          /*reserved*/
          {
              0x00, 0x00, 0x00 /*0-2*/
          }}},
    /*sensor_3dnr_cor*/
    {
        /*r_circle*/
        { 0x04E0, 0x057C, 0x0618 /*0-2*/ }
        ,
            /*reserved*/
            0x0000,
            /*u_range*/
            {
                /*min*/
                0x0000,
                /*max*/
                0x00FF,

            },
            /*v_range*/
            {
                /*min*/
                0x0000,
                /*max*/
                0x00FF,

            },
        /*uv_factor*/
        {
            /*u_thr*/
            { 0x3F, 0x3F, 0x3F, 0x3F /*0-3*/ }
            ,
                /*v_thr*/
                {
                    0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
                },
                /*u_div*/
                {
                    0x01, 0x01, 0x01, 0x01 /*0-3*/
                },
            /*v_div*/
            {
                0x01, 0x01, 0x01, 0x01 /*0-3*/
            }
        }
    }
}
,
    /*param1.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param2.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param3.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param4.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param5.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param6.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param7.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param8.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param9.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param10.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param11.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param12.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param13.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param14.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param15.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param16.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param17.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param18.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param19.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param20.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param21.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param22.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param23.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
    /*param24.&BasePoint=1&*/
    /*v21_sensor_3dnr_level*/
    {/*bypass*/
     0x00,
     /*fusion_mode*/
     0x00,
     /*filter_swt_en*/
     0x01,
     /*reserved*/
     0x00,
     /*yuv_cfg*/
     {/*grad_wgt_polyline*/
      {
          0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
          0x7F /*0-10*/
      },
      /*reserved*/
      0x00,
      /*y_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x05,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*u_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }},
      /*v_cfg*/
      {/*src_wgt*/
       0x80,
       /*nr_thr*/
       0x03,
       /*nr_wgt*/
       0xFF,
       /*thr_polyline*/
       {
           0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 /*0-8*/
       },
       /*gain_polyline*/
       {
           0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F /*0-8*/
       },
       /*reserved*/
       {
           0x00, 0x00, 0x00 /*0-2*/
       }}},
     /*sensor_3dnr_cor*/
     {/*r_circle*/
      {
          0x04E0, 0x057C, 0x0618 /*0-2*/
      },
      /*reserved*/
      0x0000,
      /*u_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*v_range*/
      {
          /*min*/
          0x0000,
          /*max*/
          0x00FF,

      },
      /*uv_factor*/
      {/*u_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*v_thr*/
       {
           0x3F, 0x3F, 0x3F, 0x3F /*0-3*/
       },
       /*u_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       },
       /*v_div*/
       {
           0x01, 0x01, 0x01, 0x01 /*0-3*/
       }}}},
