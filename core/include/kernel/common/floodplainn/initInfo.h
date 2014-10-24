#ifndef _ZBZ_UDI_INIT_WRAPPERS_H
	#define _ZBZ_UDI_INIT_WRAPPERS_H

	#define UDI_VERSION		0x101
	#include <udi.h>
	#include <__kstdlib/__ktypes.h>

namespace fplainn
{
	class MetaInit
	{
	public:
		MetaInit(udi_mei_init_t *meta_init_info)
		:
		initInfo(meta_init_info)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }

		/*udi_mei_enumeration_rank_func_t *getRankFn(void)
			{ return initInfo->mei_enumeration_rank; }*/

		udi_mei_ops_vec_template_t *getOpsVectorTemplate(
			udi_index_t idx)
		{
			udi_mei_ops_vec_template_t	*ret;

			for (ret=initInfo->ops_vec_template_list;
				ret != NULL && ret->meta_ops_num != 0;
				ret++)
			{
				if (ret->meta_ops_num == idx) { return ret; };
			};

			return NULL;
		}

		udi_mei_op_template_t *getOpTemplate(
			udi_index_t vectorIdx, udi_index_t opIdx)
		{
			udi_mei_ops_vec_template_t	*tmp;

			tmp = getOpsVectorTemplate(vectorIdx);
			if (tmp == NULL) { return NULL; };
			return getOpTemplate(tmp, opIdx);
		}

		udi_mei_op_template_t *getOpTemplate(
			udi_mei_ops_vec_template_t *opsVectorTemplate,
			udi_index_t idx)
		{
			udi_mei_op_template_t		*ret;
			uarch_t				i;

			for (i=1, ret=opsVectorTemplate->op_template_list;
				ret != NULL && ret->op_name != NULL;
				ret++, i++)
			{
				if (i == idx) { return ret; };
			};

			return NULL;
		}

	public:
		udi_mei_init_t		*initInfo;
	};

	class DriverInit
	{
	public:
		DriverInit(udi_init_t *driver_init_info)
		:
		initInfo(driver_init_info)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }

		const udi_ops_init_t *getOpsInit(udi_index_t ops_idx)
		{
			const udi_ops_init_t		*ret;

			ret = initInfo->ops_init_list;
			for (; ret != NULL && ret->ops_idx != 0; ret++)
			{
				if (ret->ops_idx == ops_idx) {
					return ret;
				};
			};

			return NULL;
		}

		const udi_cb_init_t *getCbInit(udi_index_t cb_idx)
		{
			const udi_cb_init_t		*ret;

			ret = initInfo->cb_init_list;
			for (; ret != NULL && ret->cb_idx != 0; ret++)
			{
				if (ret->cb_idx == cb_idx) {
					return ret;
				};
			};

			return NULL;
		}

		const udi_secondary_init_t *getSecondaryInit(udi_index_t rgn_idx)
		{
			const udi_secondary_init_t	*ret;

			ret = initInfo->secondary_init_list;
			for (; ret != NULL && ret->region_idx != 0; ret++)
			{
				if (ret->region_idx == rgn_idx) {
					return ret;
				};
			};

			return NULL;
		}

	public:
		udi_init_t		*initInfo;
	};
}

#endif
