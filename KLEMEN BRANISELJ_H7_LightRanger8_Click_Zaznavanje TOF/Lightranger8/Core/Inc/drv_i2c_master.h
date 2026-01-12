/****************************************************************************
 **
 ** Copyright (C) 2023 MikroElektronika d.o.o.
 ** Contact: https://www.mikroe.com/contact
 **
 ** This file is part of the mikroSDK package
 **
 ** Commercial License Usage
 **
 ** Licensees holding valid commercial NECTO compilers AI licenses may use this
 ** file in accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The MikroElektronika Company.
 ** For licensing terms and conditions see
 ** https://www.mikroe.com/legal/software-license-agreement.
 ** For further information use the contact form at
 ** https://www.mikroe.com/contact.
 **
 **
 ** GNU Lesser General Public License Usage
 **
 ** Alternatively, this file may be used for
 ** non-commercial projects under the terms of the GNU Lesser
 ** General Public License version 3 as published by the Free Software
 ** Foundation: https://www.gnu.org/licenses/lgpl-3.0.html.
 **
 ** The above copyright notice and this permission notice shall be
 ** included in all copies or substantial portions of the Software.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 ** OF MERCHANTABILITY, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 ** TO THE WARRANTIES FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 ** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 ** DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 ** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 ** OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **
 ****************************************************************************/
 #ifndef _DRV_I2C_MASTER_H_
 #define _DRV_I2C_MASTER_H_

 #ifdef __cplusplus
 extern "C"{
 #endif

 #include "drv_name.h"
 #include "hal_i2c_master.h"

 typedef enum
 {
  I2C_MASTER_SUCCESS = 0,
  I2C_MASTER_ERROR = (-1)
 } i2c_master_err_t;

 typedef enum
 {
  I2C_MASTER_SPEED_STANDARD = 0,
  I2C_MASTER_SPEED_FULL,
  I2C_MASTER_SPEED_FAST
 } i2c_master_speed_t;

 typedef struct
 {
  uint8_t addr;
  pin_name_t sda;
  pin_name_t scl;
  uint32_t speed;
  uint16_t timeout_pass_count;
 } i2c_master_config_t;

 typedef struct
 {
	 I2C_HandleTypeDef handle;
  i2c_master_config_t config;
 } i2c_master_t;

 void i2c_master_configure_default( i2c_master_config_t *config );

 err_t i2c_master_open( i2c_master_t *obj, i2c_master_config_t *config );

 err_t i2c_master_set_speed( i2c_master_t *obj, uint32_t speed );

 err_t i2c_master_set_timeout( i2c_master_t *obj, uint16_t timeout_pass_count );

 err_t i2c_master_set_slave_address( i2c_master_t *obj, uint8_t address );

 err_t i2c_master_write( i2c_master_t *obj, uint8_t *write_data_buf, size_t len_write_data );

 err_t i2c_master_read( i2c_master_t *obj, uint8_t *read_data_buf, size_t len_read_data );

 err_t i2c_master_write_then_read( i2c_master_t *obj, uint8_t *write_data_buf, size_t len_write_data, uint8_t *read_data_buf, size_t len_read_data );

 err_t i2c_master_close( i2c_master_t *obj );
  // drvi2cgroup // drvgroup // pergroup

 #ifdef __cplusplus
 }
 #endif

 #endif // _DRV_I2C_MASTER_H_
 // ------------------------------------------------------------------------- END
