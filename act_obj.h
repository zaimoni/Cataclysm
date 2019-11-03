#ifndef ACT_OBJ_H
#define ACT_OBJ_H 1

// abstract base class for a schedulable action
namespace cataclysm {

	class action
	{
	protected:
		action() = default;
		action(const action& src) = delete;
		action(action&& src) = delete;
		action& operator=(const action& src) = delete;
		action& operator=(action&& src) = delete;
	public:
		virtual ~action() = default;
		virtual bool IsLegal() const = 0;		// can be scheduled
		virtual bool IsPerformable() const = 0;	// can be done "now"
		virtual void Perform() const = 0;		// do it
	};

}

#endif
