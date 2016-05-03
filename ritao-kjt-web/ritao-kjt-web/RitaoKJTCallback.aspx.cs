using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Text;
using Newtonsoft.Json.Linq;
using System.Data;
using System.Data.SqlClient;
using ritao_kjt_web.Mod;

namespace ritao_kjt_web
{
    public partial class RitaoKJTCallback : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
            System.IO.Stream s = Request.InputStream;
            int count = 0;
            byte[] buffer = new byte[1024];
            StringBuilder builder = new StringBuilder();
            while ((count = s.Read(buffer, 0, 1024)) > 0)
            {
                builder.Append(Encoding.UTF8.GetString(buffer, 0, count));
            }
            s.Flush();
            s.Close();
            s.Dispose();

            string str = builder.ToString();

            //string str = Request.Form["data"];
            string method = Request.Form["method"];
            string data = HttpUtility.UrlDecode(Request.Form["data"]);

            // 记录到系统日志
            string strSql = "insert into 系统日志(用户名称, 日志类别, 日志日期, 日志内容, 日志备注, 创建日期) " +
                "values ('跨境通回调', '信息', '" + DateTime.Now.ToString() + "', '" + method + "', '', '" + DateTime.Now.ToString() + "')";
            SqlOpt.execSql(strSql);

            int status = 0;
            // 修改为Order.SOOutputCustoms
            //if (String.Compare(method, "Order.SOOutputWarehouse") == 0)
            //    status = OrderSOOutputWarehouse(data);
            if (String.Compare(method, "Order.SOOutputCustoms") == 0)
                status = OrderSOOutputCustoms(data);
            else if (String.Compare(method, "Inventory.ChannelQ4SAdjustRequest") == 0)
                status = InventoryChannelQ4SAdjustRequest(data);

            if (status > 0)
            {
                Response.Write("SUCCESS");
                Response.End();
                return;
            }

            //int index = str.IndexOf("&");
            //string[] arr = { };
            //if (index != -1)        // 参数大于1个
            //{
            //    arr = str.Split('&');
            //    for (int i = 0; i < arr.Length; i++)
            //    {
            //        int equalindex = arr[i].IndexOf('=');
            //        string paramN = arr[i].Substring(0, equalindex);
            //        string paramV = arr[i].Substring(equalindex + 1);

            //        if (paramN == "data")
            //        {
            //            string dataValue = HttpUtility.UrlDecode(paramV);

            //            // 解析json
            //            JObject obj = JObject.Parse(dataValue);
            //            string commitTime = (string)obj["CommitTime"];
            //            string merchantOrderID = (string)obj["MerchantOrderID"];
            //            string shipTypeID = (string)obj["ShipTypeID"];
            //            string trackingNumber = (string)obj["TrackingNumber"];

            //            // 写入数据库
            //            string strSql = "update 订单 set 发货状态=1, 物流公司='" + shipTypeID + "', 运单号='" + trackingNumber + "', 发票内容='" + commitTime + "' where 订单号='" + merchantOrderID + "'";

            //            if (SqlOpt.execSql(strSql) > 0)
            //            {
            //                Response.Write("SUCCESS");
            //                Response.End();
            //                return;
            //            }
            //        }
            //    }
            //}
            //else        // 参数少于或等于1项
            //{
            //    int equalindex = str.IndexOf('=');
            //    if (equalindex != -1)
            //    {
            //        string paramN = str.Substring(0, equalindex);
            //        string paramV = str.Substring(equalindex + 1);
            //        // ....
            //    }
            //}

            Response.Write("FAILURE");
            Response.End();
        }

        // KJT 通知商户订单出库
        private int OrderSOOutputWarehouse(string data)
        {
            // 解析json
            JObject obj = JObject.Parse(data);
            string commitTime = (string)obj["CommitTime"];
            string merchantOrderID = (string)obj["MerchantOrderID"];
            string shipTypeID = (string)obj["ShipTypeID"];
            string trackingNumber = (string)obj["TrackingNumber"];

            // 操作数据库
            string strSql = "update 订单 set 发货状态=1, 物流公司='" + shipTypeID + "', 运单号='" + trackingNumber + "', 发票内容='" + commitTime + "' where 订单号='" + merchantOrderID + "'";
            return SqlOpt.execSql(strSql);
        }

        // KJT 通知分销渠道订单已出关区
        private int OrderSOOutputCustoms(string data)
        {
            // 解析json
            JObject obj = JObject.Parse(data);
            string commitTime = (string)obj["CommitTime"];
            string merchantOrderID = (string)obj["MerchantOrderID"];
            string shipTypeID = (string)obj["ShipTypeID"];
            string trackingNumber = (string)obj["TrackingNumber"];
            string status = (string)obj["Status"];
            string message = (string)obj["Message"];

            // 操作数据库
            if (status == "1")
            {
                string strSql = "update 订单 set 发货状态=1, 物流公司='" + shipTypeID + "', 运单号='" + trackingNumber + "', 发票内容='" + commitTime + "' where 订单号='" + merchantOrderID + "'";
                return SqlOpt.execSql(strSql);
            }
            // 记录到系统日志
            else
            {
                string strSql = "insert into 系统日志(用户名称, 日志类别, 日志日期, 日志内容, 日志备注, 创建日期) " +
                    "values ('跨境通回调', '错误', '" + commitTime + "', '" + message + "', 'KJT 通知分销渠道订单已出关区', '" + DateTime.Now.ToString() + "')";
                SqlOpt.execSql(strSql);
                return -1;
            }

            //return -1;
        }

        // KJT 通知分销渠道库存调整
        private int InventoryChannelQ4SAdjustRequest(string data)
        {
            // 解析json
            JObject obj = JObject.Parse(data);
            string sysNo = (string)obj["SysNo"];
            int saleChannelSysNo = (int)obj["SaleChannelSysNo"];

            string items = obj["Items"].ToString();
            JArray array = JArray.Parse(items);
            bool b = true;
            foreach (var item in array)
            {
                JObject itemObject = JObject.Parse(item.ToString());
                string productID = (string)itemObject["ProductID"];
                int qty = (int)itemObject["Qty"];
                int wareHouseID = (int)itemObject["WareHouseID"];

                // 操作数据库

                // 查找商品KID
                //string strSql = "select 商品KID from 商品 where 商品编码='" + productID + "'";
                //int productKIDERP = SqlOpt.getIntFieldBySQL(strSql);

                int productKIDERP = 0;      // 商品KID
                string wareHouseType = "";
                // 获取商品信息 
                string strSql = "select * from 商品 where 商品编码='" + productID + "'";
                DataTable dtProduct = SqlOpt.getDataTableBySQL(strSql);
                if (dtProduct.Rows.Count > 0)
                {
                    productKIDERP = int.Parse(dtProduct.Rows[0]["商品KID"].ToString());
                    string p28 = dtProduct.Rows[0]["p28"].ToString();
                    if (p28 == "保税")
                        wareHouseType = "1";
                    else if (p28 == "直邮")
                        wareHouseType = "3";
                }

                // 查找仓库KID
                strSql = "select 仓库KID from 仓库 where 仓库编号='" + wareHouseID.ToString() + "' and 仓库类别='" + wareHouseType + "'";
                int wareHouseKIDERP = SqlOpt.getIntFieldBySQL(strSql);

                if (productKIDERP > 0 && wareHouseKIDERP > 0)
                {
                    // 如果数量>0，修改库存数量；如果数量<0，修改待发数量
                    if (qty >= 0)
                        strSql = "update 仓库库存 set 库存数量=库存数量+(" + qty.ToString() + ") where 商品ID=" + productKIDERP.ToString() + " and 仓库ID=" + wareHouseKIDERP.ToString();
                    else
                        strSql = "update 仓库库存 set 待发数量=待发数量+(" + qty.ToString() + ") where 商品ID=" + productKIDERP.ToString() + " and 仓库ID=" + wareHouseKIDERP.ToString();

                    if (SqlOpt.execSql(strSql) < 1)
                        b = false;

                    // 增加“出入库单”记录
                    int id1 = 0;
                    if (b)
                    {
                        Random rand = new Random();
                        int type = qty >= 0 ? 2 : 1;
                        string number = DateTime.Now.ToString("yyyyMMddHHmmssfffffff") + rand.Next(1000, 9999).ToString();
                        strSql = "insert into 出入库单(出入库单编号, 仓库ID, 合同ID, 发货单ID, 出入库单据类别ID, 制单日期, 货物日期, 出入库日期, 制单人ID, 往来单位ID, " +
                            "往来单位联系人, 往来单位电话, 货物地址, 运输方式, 审批人ID, 审批日期, 单据账期, 单据金额, 单据已核销金额, 单据状态, " +
                            "单据备注, 创建日期, 商品合计金额, 仓库编码 " +
                            ") values (" +
                            "'" + number + "', " + wareHouseKIDERP.ToString() + ", 0, 0, " + type.ToString() + ", '" + DateTime.Now.ToString() + "', '" + DateTime.Now.ToString() + "', '" + DateTime.Now.ToString() + "', 0, 0, " +
                            "'', '', '', '', '', '" + DateTime.Now.ToString() + "', 0, 0.0, 0.0, 0, " +
                            "'', '" + DateTime.Now.ToString() + "', 0.0, ''" +
                            ") select @@IDENTITY AS 'IDENTITY'";
                        id1 = SqlOpt.execInsertSQL(strSql);
                        if (id1 < 1)
                            b = false;
                    }

                    // 增加“出入库单明细”记录
                    if (b && id1 > 0)
                    {
                        if (dtProduct.Rows.Count > 0)
                        {
                            Random rand = new Random();
                            int type = qty >= 0 ? 2 : 1;
                            string number = DateTime.Now.ToString("yyyyMMddHHmmssfffffff") + rand.Next(1000, 9999).ToString();
                            strSql = "insert into 出入库单明细(出入库单ID, 商品ID, 库存商品ID, 库存商品编号, 商品名称, 商品规格, 商品颜色, 计量单位, 商品单价, 商品数量, " +
                                "供应商ID, large_qty, orgi_price, sale_price" +
                                ") values (" +
                                "'" + id1 + "', " + productKIDERP.ToString() + ", " + productKIDERP.ToString() + ", '" + dtProduct.Rows[0]["商品编码"] + "', '" + dtProduct.Rows[0]["商品名称"] + "', '" + dtProduct.Rows[0]["商品规格"] + "', '" + dtProduct.Rows[0]["p24"] + "', '" + dtProduct.Rows[0]["计税单位"] + "', " + dtProduct.Rows[0]["销售价"] + ", " + qty.ToString() + ", " +
                                "0, " + qty.ToString() + "," + dtProduct.Rows[0]["销售价"] + ", " + dtProduct.Rows[0]["销售价"] +
                                ") select @@IDENTITY AS 'IDENTITY'";
                            if (SqlOpt.execInsertSQL(strSql) < 1)
                                b = false;
                        }
                        else
                            b = false;

                    }
                }
                else
                    b = false;
            }

            if (b)
                return 1;
            return 0;
        }
    }
}