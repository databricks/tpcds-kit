/* 
 * Legal Notice 
 * 
 * This document and associated source code (the "Work") is a part of a 
 * benchmark specification maintained by the TPC. 
 * 
 * The TPC reserves all right, title, and interest to the Work as provided 
 * under U.S. and international laws, including without limitation all patent 
 * and trademark rights therein. 
 * 
 * No Warranty 
 * 
 * 1.1 TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THE INFORMATION 
 *     CONTAINED HEREIN IS PROVIDED "AS IS" AND WITH ALL FAULTS, AND THE 
 *     AUTHORS AND DEVELOPERS OF THE WORK HEREBY DISCLAIM ALL OTHER 
 *     WARRANTIES AND CONDITIONS, EITHER EXPRESS, IMPLIED OR STATUTORY, 
 *     INCLUDING, BUT NOT LIMITED TO, ANY (IF ANY) IMPLIED WARRANTIES, 
 *     DUTIES OR CONDITIONS OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR 
 *     PURPOSE, OF ACCURACY OR COMPLETENESS OF RESPONSES, OF RESULTS, OF 
 *     WORKMANLIKE EFFORT, OF LACK OF VIRUSES, AND OF LACK OF NEGLIGENCE. 
 *     ALSO, THERE IS NO WARRANTY OR CONDITION OF TITLE, QUIET ENJOYMENT, 
 *     QUIET POSSESSION, CORRESPONDENCE TO DESCRIPTION OR NON-INFRINGEMENT 
 *     WITH REGARD TO THE WORK. 
 * 1.2 IN NO EVENT WILL ANY AUTHOR OR DEVELOPER OF THE WORK BE LIABLE TO 
 *     ANY OTHER PARTY FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO THE 
 *     COST OF PROCURING SUBSTITUTE GOODS OR SERVICES, LOST PROFITS, LOSS 
 *     OF USE, LOSS OF DATA, OR ANY INCIDENTAL, CONSEQUENTIAL, DIRECT, 
 *     INDIRECT, OR SPECIAL DAMAGES WHETHER UNDER CONTRACT, TORT, WARRANTY,
 *     OR OTHERWISE, ARISING IN ANY WAY OUT OF THIS OR ANY OTHER AGREEMENT 
 *     RELATING TO THE WORK, WHETHER OR NOT SUCH AUTHOR OR DEVELOPER HAD 
 *     ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. 
 * 
 * Contributors:
 * Gradient Systems
 */ 
#include "config.h"
#include "porting.h"
#include <stdio.h>
#include "decimal.h"
#include "pricing.h"
#include "constants.h"
#include "columns.h"
#include "error_msg.h"
#include "dist.h"
#include "driver.h"
#include "genrand.h"
#include "w_web_sales.h"
#include "s_pline.h"

#define MAX_LIMIT	12
static ds_limits_t	aPriceLimits[MAX_LIMIT] =
{
	{CS_PRICING, CS_QUANTITY_MAX, CS_MARKUP_MAX, CS_DISCOUNT_MAX, CS_WHOLESALE_MAX, CS_COUPON_MAX},
	{SS_PRICING, SS_QUANTITY_MAX, SS_MARKUP_MAX, SS_DISCOUNT_MAX, SS_WHOLESALE_MAX, SS_COUPON_MAX},
	{WS_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX},
	{CR_PRICING, CS_QUANTITY_MAX, CS_MARKUP_MAX, CS_DISCOUNT_MAX, CS_WHOLESALE_MAX, CS_COUPON_MAX},
	{SR_PRICING, SS_QUANTITY_MAX, SS_MARKUP_MAX, SS_DISCOUNT_MAX, SS_WHOLESALE_MAX, SS_COUPON_MAX},
	{WR_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX},
	{S_PLINE_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX},
	{S_CLIN_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX},
	{S_WLIN_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX},
	{S_SRET_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX},
	{S_CRET_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX},
	{S_WRET_PRICING, WS_QUANTITY_MAX, WS_MARKUP_MAX, WS_DISCOUNT_MAX, WS_WHOLESALE_MAX, WS_COUPON_MAX}

};
typedef struct {
        int nQuantity;
        decimal_t dMarkUp;
        decimal_t dDiscount;
        decimal_t dWholesale;
} ds_limits_dec_t;
static ds_limits_dec_t aPriceLimitsDec[MAX_LIMIT];

/*
* Routine: set_pricing(int nTabId, ds_pricing_t *pPricing)
* Purpose: handle the various pricing calculations for the fact tables
* Notes:
*	the RNG usage is not kept in sync between sales pricing and returns pricing. If the calculations look wrong, it may 
*	be necessary to "waste" some RNG calls on one side or the other to bring things back in line
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
* TODO: None
*/
void set_pricing(int nTabId, ds_pricing_t *pPricing)
{
	static int init = 0,
		nQuantityMin = 1;
	static decimal_t dQuantity, dMarkupMin, dDiscountMin, dWholesaleMin, dCouponMin,
		 dZero, dOneHalf, d9pct, dOne, dTemp, dHundred;
	decimal_t dMarkup, dCoupon, dShipping, dDiscount, dTemp2;
	int i,
		nCashPct,
		nCreditPct,
		nCouponUsage;
        ds_limits_dec_t *limits;
	
	if (!init)
	{
		strtodec(&dMarkupMin, "0.00");
		strtodec(&dDiscountMin, "0.00");
		strtodec(&dWholesaleMin, "1.00");
		strtodec(&dCouponMin, "0.00");
		strtodec(&dZero, "0.00");
		strtodec(&dOneHalf, "0.50");
		strtodec(&d9pct, "0.09");
		strtodec(&dWholesaleMin, "1.00");
		strtodec(&dHundred, "100.00");
		strtodec(&dOne, "1.00");
		for (i=0; i < MAX_LIMIT; i++)
                {
                    strtodec(&aPriceLimitsDec[i].dDiscount, aPriceLimits[i].szDiscount);
                    strtodec(&aPriceLimitsDec[i].dWholesale, aPriceLimits[i].szWholesale);
                    strtodec(&aPriceLimitsDec[i].dMarkUp, aPriceLimits[i].szMarkUp);
                    aPriceLimitsDec[i].nQuantity = atoi(aPriceLimits[i].szQuantity);
                }

		init = 1;
	}
	switch(nTabId) {
	case CS_PRICING:
	case CR_PRICING: limits = &aPriceLimitsDec[0]; break;
	case SS_PRICING:
	case SR_PRICING: limits = &aPriceLimitsDec[1]; break;
	case WS_PRICING:
	case WR_PRICING:
        default: limits = &aPriceLimitsDec[2]; break;
        }
	switch(nTabId)
	{
	case SS_PRICING:
	case CS_PRICING:
	case WS_PRICING:
	case S_PLINE_PRICING:
	case S_CLIN_PRICING:
	case S_WLIN_PRICING:
		genrand_integer(&pPricing->quantity, DIST_UNIFORM, nQuantityMin, limits->nQuantity, 0, nTabId);
		itodec(&dQuantity, pPricing->quantity);
		genrand_decimal(&pPricing->wholesale_cost, DIST_UNIFORM, &dWholesaleMin, &limits->dWholesale, NULL, nTabId);

		/* ext_wholesale_cost = wholesale_cost * quantity */
		decimal_t_op_mult0(&pPricing->ext_wholesale_cost, &dQuantity, &pPricing->wholesale_cost);
		
		/* list_price = wholesale_cost * (1 + markup) */
		genrand_decimal(&dMarkup, DIST_UNIFORM, &dMarkupMin, &limits->dMarkUp, NULL, nTabId);
		decimal_t_op_plus(&dMarkup, &dMarkup, &dOne);
		decimal_t_op_mult2(&pPricing->list_price, &pPricing->wholesale_cost, &dMarkup);
		
		/* sales_price = list_price * (1 - discount)*/
		genrand_decimal(&dDiscount, DIST_UNIFORM, &dDiscountMin, &limits->dDiscount, NULL, nTabId);
		NegateDecimal(&dDiscount);
		decimal_t_op_plus(&pPricing->ext_discount_amt, &dDiscount, &dOne);
		decimal_t_op_mult2(&pPricing->sales_price, &pPricing->list_price, &pPricing->ext_discount_amt);
		
		/* ext_list_price = list_price * quantity */
		decimal_t_op_mult0(&pPricing->ext_list_price, &pPricing->list_price, &dQuantity);
		
		/* ext_sales_price = sales_price * quantity */
		decimal_t_op_mult0(&pPricing->ext_sales_price, &pPricing->sales_price, &dQuantity);
		
		/* ext_discount_amt = ext_list_price - ext_sales_price */
		decimal_t_op_minus(&pPricing->ext_discount_amt, &pPricing->ext_list_price, &pPricing->ext_sales_price);
		
		/* coupon_amt = ext_sales_price * coupon */
		genrand_decimal(&dCoupon, DIST_UNIFORM, &dZero, &dOne, NULL, nTabId);
		genrand_integer(&nCouponUsage, DIST_UNIFORM, 1, 100, 0, nTabId);
		if (nCouponUsage <= 20)	/* 20% of sales employ a coupon */
			decimal_t_op_mult2(&pPricing->coupon_amt, &pPricing->ext_sales_price, &dCoupon);
		else
			memcpy(&pPricing->coupon_amt, &dZero, sizeof(decimal_t));
		
		/* net_paid = ext_sales_price - coupon_amt */
		decimal_t_op_minus(&pPricing->net_paid, &pPricing->ext_sales_price, &pPricing->coupon_amt);
		
		/* shipping_cost = list_price * shipping */
		genrand_decimal(&dShipping, DIST_UNIFORM, &dZero, &dOneHalf, NULL, nTabId);
		decimal_t_op_mult2(&pPricing->ship_cost, &pPricing->list_price, &dShipping);

      /* ext_shipping_cost = shipping_cost * quantity */
		decimal_t_op_mult0(&pPricing->ext_ship_cost, &pPricing->ship_cost, &dQuantity);
		
		/* net_paid_inc_ship = net_paid + ext_shipping_cost */
		decimal_t_op_plus(&pPricing->net_paid_inc_ship, &pPricing->net_paid, &pPricing->ext_ship_cost);
		
		/* ext_tax = tax * net_paid */
		genrand_decimal(&pPricing->tax_pct, DIST_UNIFORM, &dZero, &d9pct, NULL, nTabId);
		decimal_t_op_mult2(&pPricing->ext_tax, &pPricing->net_paid, &pPricing->tax_pct);
		
		/* net_paid_inc_tax = net_paid + ext_tax */
		decimal_t_op_plus(&pPricing->net_paid_inc_tax, &pPricing->net_paid, &pPricing->ext_tax);
		
		/* net_paid_inc_ship_tax = net_paid_inc_tax + ext_shipping_cost */
		decimal_t_op_plus(&pPricing->net_paid_inc_ship_tax, &pPricing->net_paid_inc_ship, &pPricing->ext_tax);
		
		/* net_profit = net_paid - ext_wholesale_cost */
		decimal_t_op_minus(&pPricing->net_profit, &pPricing->net_paid, &pPricing->ext_wholesale_cost);
		break;
	case CR_PRICING:
	case SR_PRICING:
	case WR_PRICING:
		/* quantity is determined before we are called */
		/* ext_wholesale_cost = wholesale_cost * quantity */
		itodec(&dQuantity, pPricing->quantity);
		decimal_t_op_mult0(&pPricing->ext_wholesale_cost, &dQuantity, &pPricing->wholesale_cost);
		
		/* ext_list_price = list_price * quantity */
		decimal_t_op_mult0(&pPricing->ext_list_price, &pPricing->list_price, &dQuantity);
		
		/* ext_sales_price = sales_price * quantity */
		decimal_t_op_mult0(&pPricing->ext_sales_price, &pPricing->sales_price, &dQuantity);
		
		/* net_paid = ext_list_price (couppons don't effect returns) */
		memcpy(&pPricing->net_paid, &pPricing->ext_sales_price, sizeof(decimal_t));
		
		/* shipping_cost = list_price * shipping */
		genrand_decimal(&dShipping, DIST_UNIFORM, &dZero, &dOneHalf, NULL, nTabId);
		decimal_t_op_mult2(&pPricing->ship_cost, &pPricing->list_price, &dShipping);

      /* ext_shipping_cost = shipping_cost * quantity */
		decimal_t_op_mult0(&pPricing->ext_ship_cost, &pPricing->ship_cost, &dQuantity);
				
		/* net_paid_inc_ship = net_paid + ext_shipping_cost */
		decimal_t_op_plus(&pPricing->net_paid_inc_ship, &pPricing->net_paid, &pPricing->ext_ship_cost);
		
		/* ext_tax = tax * net_paid */
		decimal_t_op_mult2(&pPricing->ext_tax, &pPricing->net_paid, &pPricing->tax_pct);
		
		/* net_paid_inc_tax = net_paid + ext_tax */
		decimal_t_op_plus(&pPricing->net_paid_inc_tax, &pPricing->net_paid, &pPricing->ext_tax);
		
		/* net_paid_inc_ship_tax = net_paid_inc_tax + ext_shipping_cost */
		decimal_t_op_plus(&pPricing->net_paid_inc_ship_tax, &pPricing->net_paid_inc_ship, &pPricing->ext_tax);
		
		/* net_profit = net_paid - ext_wholesale_cost */
		decimal_t_op_minus(&pPricing->net_profit, &pPricing->net_paid, &pPricing->ext_wholesale_cost);
		
		/* see to it that the returned amounts add up to the total returned */
		/* allocate some of return to cash */
		genrand_integer(&nCashPct, DIST_UNIFORM, 0, 100, 0, nTabId);
		itodec(&dTemp, nCashPct);
                dTemp.precision = 2;
		decimal_t_op_mult2(&pPricing->refunded_cash, &dTemp, &pPricing->net_paid);
		
		/* allocate some to reversed charges */
		genrand_integer(&nCreditPct, DIST_UNIFORM, 1, 100, 0, nTabId);
		itodec(&dTemp, nCreditPct);
                dTemp2.precision = 2;
		decimal_t_op_minus(&dTemp2, &pPricing->net_paid, &pPricing->refunded_cash);
		decimal_t_op_mult2(&pPricing->reversed_charge, &dTemp2, &dTemp);
		
		/* the rest is store credit */
		decimal_t_op_minus(&pPricing->store_credit, &pPricing->net_paid, &pPricing->reversed_charge);
		decimal_t_op_minus(&pPricing->store_credit, &pPricing->store_credit, &pPricing->refunded_cash);
		
		/* pick a fee for the return */
		genrand_decimal(&pPricing->fee, DIST_UNIFORM, &dOneHalf, &dHundred, &dZero, nTabId);
		
		/* and calculate the net effect */
		decimal_t_op_minus(&pPricing->net_loss, &pPricing->net_paid_inc_ship_tax, &pPricing->store_credit);
		decimal_t_op_minus(&pPricing->net_loss, &pPricing->net_loss, &pPricing->refunded_cash);
		decimal_t_op_minus(&pPricing->net_loss, &pPricing->net_loss, &pPricing->reversed_charge);
		decimal_t_op_plus(&pPricing->net_loss, &pPricing->net_loss, &pPricing->fee);
		break;
	}

	return;

}







