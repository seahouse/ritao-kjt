using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Data;
using System.Data.SqlClient;
using ritao_kjt_web.Mod;

namespace ritao_kjt_web.Mod
{
    public class SqlOpt
    {
        public static int execSql(string strSql)
        {
            SqlConnection con = new SqlConnection();
            con.ConnectionString = SqlConn.getConn();

            SqlCommand com = new SqlCommand();
            com.Connection = con;
            com.CommandType = CommandType.Text;
            com.CommandText = strSql;
            con.Open();
            int rowsAffected = com.ExecuteNonQuery();
            con.Close();

            return rowsAffected;
        }

        // 
        public static int execInsertSQL(string strSql)
        {
            int insertId = 0;
            SqlConnection con = new SqlConnection(SqlConn.getConn());
            SqlCommand com = new SqlCommand(strSql, con);
            con.Open();
            object v = com.ExecuteScalar();
            if (v != null)
                insertId = int.Parse(v.ToString());
            con.Close();

            return insertId;
        }

        public static int getIntFieldBySQL(string strSql)
        {
            int field = 0;
            SqlConnection con = new SqlConnection();
            con.ConnectionString = SqlConn.getConn();

            SqlCommand com = new SqlCommand();
            com.Connection = con;
            com.CommandType = CommandType.Text;
            com.CommandText = strSql;
            con.Open();
            object v = com.ExecuteScalar();
            if (v != null)
                field = int.Parse(v.ToString());
            con.Close();

            return field;
        }

        public static DataTable getDataTableBySQL(string strSql)
        {
            SqlConnection con = new SqlConnection(SqlConn.getConn());
            SqlCommand com = new SqlCommand(strSql, con);
            SqlDataAdapter dataAdapter = new SqlDataAdapter(com);
            DataSet dataSet = new DataSet();
            DataTable dt = new DataTable();
            try
            {
                dataAdapter.Fill(dataSet);
                dt = dataSet.Tables[0];
            }
            catch
            {

            }
            finally
            {
                con.Close();
            }
            return dt;
        }
    }
}