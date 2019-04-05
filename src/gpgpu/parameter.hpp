#pragma once

namespace brtr
{
	class parameter
	{
	public:
		parameter() = default;
		virtual ~parameter() = default;
		virtual void set_arg() const = 0;
	};

	template <typename Derived>
	class parameter_base : parameter
	{
	public:
		void set_arg() const override
		{
			static_cast<Derived*>(this)->implementation();
		}
	};

}